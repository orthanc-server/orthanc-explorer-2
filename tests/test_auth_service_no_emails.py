from playwright.sync_api import sync_playwright, Page, expect
from orthanc_api_client import OrthancApiClient
from helpers import *
import pytest
import pathlib
import time
import re

here = pathlib.Path(__file__).parent.resolve()

CONFIG_NAME = "conf-auth-service-no-emails"
COMPOSE_FILE = "docker-compose.auth-service.yml"
HAS_KEYCLOAK = True
HAS_NGINX = True

ORTHANC_CONFIG = {
    "AuthenticationEnabled": False,
    "Authorization": {
        "WebServiceRootUrl": "http://orthanc-auth-service:8000/",
        "WebServiceUsername": "share-user",
        "WebServicePassword": "change-me",
        "StandardConfigurations" : [
            "osimis-web-viewer",
            "stone-webviewer",
            "orthanc-explorer-2",
            "ohif",
            "stl"
        ],
        "TokenHttpHeaders" : [ "api-key" ],
        "CheckedLevel": "studies",
        "ExtraPermissions": [
            ["post", "^/plugins/inbox/commit$", "upload|all"],
            ["post", "^/plugins/inbox/monitor-processing$", "upload|all"]
        ]
    },
    
    "OrthancExplorer2": {
        "UiOptions": {
            "StudyListSearchMode": "search-button",

              "EnableInboxLinks": True,
              "EnableInboxLinksByEmail": False,  # TODO
              "InboxLinksEmailContentTemplate": "inbox-links-content",
              "InboxLinksEmailLayoutTemplate": "share-study-layout",
              "InboxLinksEmailTitle": "My Hospital has shared an inbox link with you",
        },
        "Keycloak" : {
            "Enable": True,
            "Url": "http://localhost/keycloak/",
            "Realm": "orthanc",
            "ClientId": "orthanc"
        },
        "Inbox": {
            "Enable": True,
            "EnableAnonymousAccess": False,
            "Title": "My Hospital",
            "IntroTextHtml": "<p>Welcome to this inbox.</p><p>Upload your DICOM files.</p>",
            # "CommitUrl": "plugins/inbox/commit",
            # "ProcessingMonitoringUrl": "plugins/inbox/monitor-processing",
            "FormFields": []
        }, 
    }
}


def test_inbox_link(page: Page, incognito_page: Page, orthanc_api: OrthancApiClient):

    with TestContext("Generate an inbox-link from a logged-in user"):
        login_keycloak(page, "admin", "change-me")
        take_screenshot(page, "inbox-after-login")
        expect(page.locator("#filter-PatientName")).to_be_visible(timeout=5000)

        # generate an inbox-link
        page.locator('#inbox-links-side-menu').click()
        page.locator('#inbox-links-email').fill("a@a.be")
        page.locator('#inbox-links-generate').click()
        inbox_link_input = page.locator('#inbox_link_txt')
        expect(inbox_link_input).to_have_value(re.compile(r"^http://.*inbox\.html"), timeout=5000)
        take_screenshot(page, "inbox-generate-link")

    with TestContext("Upload a file with this inbox-link"):
        # new isolated/incognito browser with the link
        incognito_page.goto(inbox_link_input.input_value())
        expect(incognito_page.locator('#inbox-user-name')).to_have_text("a@a.be")

        upload_file(incognito_page, here / "stimuli/TEST_1/10.dcm")

        expect(incognito_page.locator('#inbox-reload')).to_be_visible()

        with TestContext("Reload the page after upload"):
            incognito_page.locator('#inbox-reload').click()
            expect(incognito_page.locator('#inbox-user-name')).to_have_text("a@a.be")

def test_inbox_without_login(page: Page, incognito_page: Page, orthanc_api: OrthancApiClient):
    page.goto("/ui/app/inbox.html")
    expect_login_screen_displayed(page)
