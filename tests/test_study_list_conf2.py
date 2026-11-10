from playwright.sync_api import sync_playwright, Page, expect
from orthanc_api_client import OrthancApiClient
from helpers import *
import pytest
import pathlib
import re

here = pathlib.Path(__file__).parent.resolve()

CONFIG_NAME = "conf2"
COMPOSE_FILE = "docker-compose.no-auth.yml"
HAS_KEYCLOAK = False
HAS_NGINX = False


ORTHANC_CONFIG = {
    "AuthenticationEnabled": False,
    
    "OrthancExplorer2": {
        "UiOptions": {
            "StudyListSearchMode": "search-button"
        }
    }
}


def test_search_study_list(page: Page, orthanc_api: OrthancApiClient):
    orthanc_api.delete_all_content()

    orthanc_api.upload_file(here / "stimuli/TEST_1/10.dcm")
    orthanc_api.upload_file(here / "stimuli/TEST_1/12.dcm")

    expect(page.locator("#filter-PatientName")).to_be_visible()

    # search for a PatientName that exists
    page.locator("#filter-PatientName").fill("Te")
    page.locator('#search').click()
    expect(get_collapsed_studies(page)).to_have_count(1, timeout=2000)

    # search for a PatientName that does not exist
    page.locator("#filter-PatientName").fill("Arn")
    page.locator('#search').click()
    expect(get_collapsed_studies(page)).to_have_count(0, timeout=2000)

