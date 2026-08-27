from playwright.sync_api import sync_playwright, Page, expect
import pathlib
import time
import json
import requests

here = pathlib.Path(__file__).parent.resolve()


class TestContext:
    __test__ = False
    _level = 0

    def __init__(self, title):
        self.title = title

    def __enter__(self):
        print("  " * TestContext._level *2 + f">>> {self.title}")
        TestContext._level += 1
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        TestContext._level -= 1

        status = "OK" if exc_type is None else "FAILED"

        print(
            "  " * TestContext._level * 2
            + f"<<< {self.title} [{status}]"
        )

        return False

def get_collapsed_studies(page: Page):
    return page.locator(".study-row-collapsed")

def get_date_picker(page: Page, id: str):
    return page.locator(f"{id} input").first

def login_keycloak(page: Page, username: str, pwd:str):
    expect_login_screen_displayed(page)
    page.locator("#username").fill(username)
    page.locator("#password").fill(pwd)
    page.locator("#kc-login").click()

def expect_login_screen_displayed(page: Page):
    expect(page.locator("#username")).to_be_visible()
    expect(page.locator("#password")).to_be_visible()
    expect(page.locator("#kc-login")).to_be_visible()

def upload_file(page: Page, file_path):
    page.locator("#filesUpload").set_input_files(file_path)

def take_screenshot(page: Page, name):
    page.screenshot(path=here / f"screenshots/{name}.png", full_page=True)

def get_first_email_for(recipient: str, timeout=10):
    deadline = time.time() + timeout

    while time.time() < deadline:
        try:
            response = requests.get("http://localhost:1080/email")
            emails = response.json()
            for email in emails:
                if recipient in email.get("to", "")[0]['address']:
                    return email
        except Exception as ex:
            pass
        time.sleep(0.2)

    raise AssertionError(
        f"No email received for {recipient} within {timeout}s"
    )