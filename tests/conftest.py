import subprocess
import time
import json
from orthanc_api_client import OrthancApiClient

import pytest
from playwright.sync_api import sync_playwright
import pathlib
from urllib.parse import urljoin

here = pathlib.Path(__file__).parent.resolve()


ORTHANC_EXECUTABLE = here / "../../build/orthanc/Orthanc"
UI_HOST = "127.0.0.1"
UI_PORT = 8043   # by using 8043, Orthanc is also accessible behind localhost:3000 if you have started `npm run dev` which is convenient if you are running tests while developing


def start_orthanc(orthanc_config, config_name):

    orthanc_config["Name"] = config_name
    orthanc_config["StorageDirectory"] = str(here / f"./storages/{config_name}")
    orthanc_config["HttpPort"] = UI_PORT

    config_file_path = str(here / f"./configs/{config_name}.json")
    with open(config_file_path, "w") as f:
        f.write(json.dumps(orthanc_config))

    command = [
        str(ORTHANC_EXECUTABLE),
        "--verbose", 
        config_file_path,
        str(here / "configs/local-plugins.json")
    ]

    print(f"Starting Orthanc: {' '.join(command)}", flush=True)

    process = subprocess.Popen(
        command,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )

    # Wait until the UI is available
    import urllib.request

    url = f"http://{UI_HOST}:{UI_PORT}/ui/app/"

    for _ in range(100):
        try:
            urllib.request.urlopen(url, timeout=1)
            print(f"UI is ready at {url}", flush=True)
            return process
        except Exception:
            time.sleep(0.1)

    process.terminate()

    output, _ = process.communicate(timeout=5)

    raise RuntimeError(
        f"UI failed to start.\n"
        f"Output:\n{output}"
    )


@pytest.fixture(scope="module")
def ui(request):
    orthanc_config = request.module.ORTHANC_CONFIG
    config_name = request.module.CONFIG_NAME

    process = start_orthanc(orthanc_config, config_name)

    yield process

    print("Stopping Orthanc", flush=True)

    process.terminate()

    try:
        process.wait(timeout=5)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait()


def pytest_addoption(parser):
    parser.addoption(
        "--orthanc-url",
        action="store",
        default=f"http://{UI_HOST}:{UI_PORT}",
        help="URL of the app under test",
    )

    parser.addoption(
        "--orthanc-mode",
        choices=[
            "existing-orthanc",
            "orthanc-native",
            "orthanc-docker",
        ],
        default="existing-orthanc",
    )

    parser.addoption(
        "--orthanc-exe",
        action="store",
        default=str(here / "../../build/orthanc/Orthanc"),
        help="Path to the Orthanc executable",
    )

# def load_config(test_module):
#     config_file = here / test_module.UI_CONFIG

#     with open(config_file) as f:
#         return json.load(f)


# @pytest.fixture(scope="module")
# def config(request):
#     return load_config(request.module)

@pytest.fixture(scope="module")
def orthanc_api(request) -> OrthancApiClient:
    url = request.config.getoption("--orthanc-url")
    # api_http_port = config.get('HttpPort') or 8042
    # uri = f"http://{UI_HOST}:{api_http_port}"

    return OrthancApiClient(url)

@pytest.fixture(scope="module")
def browser(ui):
    with sync_playwright() as p:
        browser = p.chromium.launch(headless=True)
        yield browser
        browser.close()


@pytest.fixture
def page(browser, request):
    page = browser.new_page(
        base_url=urljoin(request.config.getoption("--orthanc-url"), "/ui/app/")
    )

    page.on(
        "requestfailed",
        lambda request: print(
            f"REQUEST FAILED: {request.method} {request.url}\n"
            f"  error: {request.failure}"
        )
    )

    page.on(
        "response",
        lambda response: print(
            f"HTTP {response.status}: {response.url}"
        ) if response.status >= 400 else None
    )

    page.on(
        "console",
        lambda msg: print(
            f"CONSOLE [{msg.type}]: {msg.text}"
        )
    )

    page.on(
        "pageerror",
        lambda exc: print(f"PAGE ERROR: {exc}")
    )

    # url = urljoin(request.config.getoption("--orthanc-url"), "/ui/app/")
    # print(f"\nNavigating to: {url}")

    page.goto("/", wait_until="networkidle")

    print(f"Final URL: {page.url}")

    yield page
    page.close()