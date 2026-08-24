import subprocess
import time
import json
from orthanc_api_client import OrthancApiClient

import pytest
from playwright.sync_api import sync_playwright
import pathlib

here = pathlib.Path(__file__).parent.resolve()


ORTHANC_EXECUTABLE = here / "../../build/orthanc/Orthanc"
UI_HOST = "127.0.0.1"
UI_PORT = 8099


def start_orthanc(config_file):
    command = [
        str(ORTHANC_EXECUTABLE),
        "--verbose", 
        str(here / config_file),
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
    config_file = request.module.UI_CONFIG

    process = start_orthanc(config_file)

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
        "--base-url",
        action="store",
        default=f"http://{UI_HOST}:{UI_PORT}/ui/app",
        help="URL of the app under test",
    )


def load_config(test_module):
    config_file = here / test_module.UI_CONFIG

    with open(config_file) as f:
        return json.load(f)


@pytest.fixture(scope="module")
def config(request):
    return load_config(request.module)

@pytest.fixture(scope="module")
def orthanc_api(config) -> OrthancApiClient:
    api_http_port = config.get('HttpPort') or 8042
    uri = f"http://{UI_HOST}:{api_http_port}"

    return OrthancApiClient(uri)

@pytest.fixture(scope="module")
def browser(ui):
    with sync_playwright() as p:
        browser = p.chromium.launch(headless=True)
        yield browser
        browser.close()


@pytest.fixture
def page(browser, request):
    page = browser.new_page()

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

    url = request.config.getoption("--base-url")
    print(f"\nNavigating to: {url}")

    page.goto(url, wait_until="networkidle")

    print(f"Final URL: {page.url}")

    yield page
    page.close()