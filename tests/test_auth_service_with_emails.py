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
              "EnableInboxLinksByEmail": True,
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
        "Emails": {
            "Server": {
                "Url": "http://orthanc-auth-service:8000/emails/",
                "Username" : "share-user",
                "Password" : "change-me"
            }
        }
    }
}


def test_inbox_link(page: Page, incognito_page: Page, orthanc_api: OrthancApiClient):

    with TestContext("Generate an inbox-link from a logged-in user"):
        login_keycloak(page, "admin", "change-me")
        take_screenshot(page, "inbox-after-login")
        expect(page.locator("#filter-PatientName")).to_be_visible(timeout=5000)

        # generate an inbox-link
        page.locator('#inbox-links-side-menu').click()
        page.locator('#inbox-links-email').fill("ab@ab.be")
        page.locator('#inbox-links-generate').click()

        # edit the email text in the TipTap Editor
        editor = page.locator("div.tiptap.ProseMirror")
        editor.press('End')
        editor.type('-appended-text-body-')

        page.locator('#inbox-links-email-title').fill('-modified-email-title-')

        page.locator('#inbox-links-send-email').click()

        # make sure an "email-sent" toast is displayed
        expect(page.locator("div.toast-body")).to_contain_text("Email sent")

        # make sure the mail has been received
        email = get_first_email_for("ab@ab.be")
        assert "inbox.html" in email.get('html')
        assert "-appended-text-body-" in email.get('html')
        assert "token=ey" in email.get('html')
        assert "-modified-email-title-" in email.get('subject')


    # note: testing the inbox link itself is performed in the test with no-emails
