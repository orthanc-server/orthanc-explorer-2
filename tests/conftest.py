import subprocess
import time
import json
from orthanc_api_client import OrthancApiClient
import os
import re
import pytest
from playwright.sync_api import sync_playwright
import pathlib
from urllib.parse import urljoin
import requests

here = pathlib.Path(__file__).parent.resolve()


ORTHANC_EXECUTABLE = here / "../../build/orthanc/Orthanc"


# display the test name before running it
@pytest.fixture(autouse=True)
def test_context(request):
    print()
    print("=" * 80)
    print(f"TEST: {request.node.name}")
    print("=" * 80)

    yield

    print()


# take a snapshot when a test fail
@pytest.hookimpl(hookwrapper=True)
def pytest_runtest_makereport(item, call):
    outcome = yield
    report = outcome.get_result()

    if report.when == "call" and report.failed:
        page = item.funcargs.get("page")

        if page is not None:
            screenshots_dir = here / "screenshots"
            screenshots_dir.mkdir(parents=True, exist_ok=True)
            filename = re.sub(r"[^a-zA-Z0-9_.-]", "_", item.nodeid)

            screenshot_filename = screenshots_dir / f"{filename}.png"
            page.screenshot(path=screenshot_filename, full_page=True)

            html_filename = screenshots_dir / f"{filename}.html"
            html_filename.write_text(page.content())

            print(f"\nFAILED URL: {page.url}", flush=True)
            print(f"\nScreenshot saved to: {screenshot_filename}", flush=True)
            print(f"\nHTML saved to: {screenshot_filename}", flush=True)


def wait_ui_is_available(request, orthanc_url, timeout=20):
    alive_test_urls = [[urljoin(orthanc_url, 'ui/app/'), 200]]

    if request.module.HAS_KEYCLOAK:
        alive_test_urls = [
            ["http://localhost/keycloak/realms/orthanc/protocol/openid-connect/3p-cookies/step1.html", 200],
            ["http://localhost:8000", 404],
            [urljoin(orthanc_url, 'ui/app/'), 200]
        ]

    # Wait until the UI is available
    for _ in range(100):
        success_count = 0
        for alive_test_url, expected_status in alive_test_urls:
            try:
                response = requests.get(alive_test_url, allow_redirects=False)  
                if response.status_code == expected_status:
                    success_count += 1
                print(f"Waiting for UI to be alive: Tested Url {alive_test_url} [{response.status_code} vs {expected_status}]", flush=True)
            except Exception as e:
                print(f"Waiting for UI to be alive: Exception while testing {alive_test_url}")

        if success_count == len(alive_test_urls):
            return True
        time.sleep(1)

    return False


def start_orthanc(request, orthanc_config, config_name, orthanc_exe = None, compose_file = None, orthanc_url = None):

    print(f"Generating configuration file {config_name}.json", flush=True)

    orthanc_config["Name"] = config_name

    if compose_file:
        if request.module.HAS_NGINX:
            orthanc_config["HttpPort"] = 8042 # because we are behind nginx that expects port 8042
        else:
            orthanc_config["HttpPort"] = 8043
        orthanc_config["StorageDirectory"] = "/var/lib/orthanc/db"

        if "Authorization" in orthanc_config and "WebServiceRootUrl" in orthanc_config["Authorization"]:
            orthanc_config["Authorization"]["WebServiceRootUrl"] = "http://orthanc-auth-service:8000/"
        if "OrthancExplorer2" in orthanc_config and "Emails" in orthanc_config["OrthancExplorer2"]:
            orthanc_config["OrthancExplorer2"]["Emails"]["Server"]["Url"] = "http://orthanc-auth-service:8000/emails/"

        config_file_path = str(here / f"./configs/conf-for-docker.json")

    else:    
        orthanc_config["HttpPort"] = 8043  # by using 8043, Orthanc is also accessible behind localhost:3000 if you have started `npm run dev` which is convenient if you are running tests while developing
        orthanc_config["StorageDirectory"] = str(here / f"./storages/{config_name}")

        if "Authorization" in orthanc_config and "WebServiceRootUrl" in orthanc_config["Authorization"]:
            orthanc_config["Authorization"]["WebServiceRootUrl"] = "http://localhost:8000/"
        if "OrthancExplorer2" in orthanc_config and "Emails" in orthanc_config["OrthancExplorer2"]:
            orthanc_config["OrthancExplorer2"]["Emails"]["Server"]["Url"] = "http://localhost:8000/emails/"

        config_file_path = str(here / f"./configs/{config_name}.json")


    with open(config_file_path, "w") as f:
        f.write(json.dumps(orthanc_config, indent=2))

    if orthanc_exe:

        command = [
            str(ORTHANC_EXECUTABLE),
            "--verbose", 
            config_file_path,
            str(here / "configs/local-plugins.json")
        ]

        print(f"Starting Orthanc exe: {' '.join(command)}", flush=True)

        process = subprocess.Popen(
            command,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
        )

        if wait_ui_is_available(request, orthanc_url):
            return process

        process.terminate()

        output, _ = process.communicate(timeout=5)

        raise RuntimeError(
            f"UI failed to start.\n"
            f"Output:\n{output}"
        )

    elif compose_file:
        compose_full_path = os.path.join(here / 'docker', compose_file)
        subprocess.run([
            "docker", "compose", 
            "-p", config_name,
            "down", "-v"])

        command = [
            "docker", "compose", 
            "-p", config_name,
            "-f", compose_full_path,
            "up", "-d", "--build"
        ]

        print(f"Starting Orthanc with docker: {' '.join(command)}", flush=True)

        process = subprocess.Popen(
            command,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
        )

        if wait_ui_is_available(request, orthanc_url=orthanc_url, timeout=250):
            return process

        process.terminate()

        output, _ = process.communicate(timeout=5)

        raise RuntimeError(
            f"UI failed to start.\n"
            f"Output:\n{output}"
        )



@pytest.fixture(scope="module")
def ui(request, orthanc_url):
    orthanc_config = request.module.ORTHANC_CONFIG
    config_name = request.module.CONFIG_NAME

    orthanc_mode = request.config.getoption("--orthanc-mode")

    if orthanc_mode == 'orthanc-docker':
        process = start_orthanc(request, orthanc_config, config_name, compose_file=request.module.COMPOSE_FILE, orthanc_url=orthanc_url)
    elif orthanc_mode == 'orthanc-native':
        process = start_orthanc(request, orthanc_config, config_name, orthanc_exe=request.config.getoption("--orthanc-exe"), orthanc_url=orthanc_url)
    else:
        yield None
        return

    yield process

    if orthanc_mode == 'orthanc-native':
        print("Stopping Orthanc", flush=True)

        process.terminate()

    elif orthanc_mode == 'orthanc-docker':
        print("Stopping Docker compose", flush=True)

        subprocess.run([
            "docker", "compose", 
            "-p", config_name,
            "down", "-v"])

    try:
        process.wait(timeout=5)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait()


def pytest_addoption(parser):
    parser.addoption(
        "--orthanc-url",
        action="store",
        default=f"http://localhost:8043",
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


@pytest.fixture(scope="module")
def orthanc_url(request) -> OrthancApiClient:
    orthanc_mode = request.config.getoption("--orthanc-mode")
    if orthanc_mode == "orthanc-docker":
        if request.module.HAS_KEYCLOAK:
            return "http://localhost/orthanc/"
        else:
            return "http://localhost:8043"
    else:
        return request.config.getoption("--orthanc-url").rstrip("/") + "/"


@pytest.fixture(scope="module")
def orthanc_api(request, orthanc_url) -> OrthancApiClient:
    return OrthancApiClient(orthanc_url)

@pytest.fixture(scope="module")
def browser(ui):
    with sync_playwright() as p:
        browser = p.chromium.launch(headless=True)
        yield browser
        browser.close()

@pytest.fixture
def page(browser, request, orthanc_url):
    page = browser.new_page(
        base_url=orthanc_url
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

    page.goto("/", wait_until="networkidle")

    print(f"Final URL: {page.url}")

    yield page
    page.close()


@pytest.fixture
def incognito_page(browser):
    context = browser.new_context()
    page = context.new_page()

    yield page

    context.close()