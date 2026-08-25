from playwright.sync_api import sync_playwright, Page, expect
from orthanc_api_client import OrthancApiClient
from helpers import *
import pytest
import pathlib
import time

here = pathlib.Path(__file__).parent.resolve()

CONFIG_NAME = "Conf1"

ORTHANC_CONFIG = {
    "AuthenticationEnabled": False,

    "OrthancExplorer2": {
        "UiOptions": {
            "StudyListSearchMode": "search-as-you-type",
            "StudyListSearchAsYouTypeDelay": 100
        }
    }
}


def test_search_study_list(page: Page, orthanc_api: OrthancApiClient):
    orthanc_api.delete_all_content()

    orthanc_api.upload_file(here / "stimuli/TEST_1/10.dcm")
    orthanc_api.upload_file(here / "stimuli/TEST_1/12.dcm")

    expect(page.locator("#filter-PatientName")).to_be_visible()

    page.locator("#filter-PatientName").fill("Tes")
    expect(get_collapsed_studies(page)).to_have_count(1, timeout=2000)

    page.locator("#filter-PatientName").fill("Arn")
    page.screenshot(path=here / "screenshots/debug.png", full_page=True)
    expect(get_collapsed_studies(page)).to_have_count(0, timeout=2000)

    # open study list from URL (with PatientName)
    page.goto('filtered-studies?PatientName=Test&order-by=Metadata,LastUpdate,DESC')
    expect(page.locator("#filter-PatientName")).to_have_value("Test")
    expect(get_collapsed_studies(page)).to_have_count(1, timeout=2000)

    # open study list from URL (with PatientID)
    page.goto('filtered-studies?PatientID=TEST_1&order-by=Metadata,LastUpdate,DESC')
    expect(page.locator("#filter-PatientID")).to_have_value("TEST_1")
    expect(get_collapsed_studies(page)).to_have_count(1, timeout=2000)

    # open study list from URL (no match)
    page.goto('filtered-studies?PatientBirthDate=20360203&order-by=Metadata,LastUpdate,DESC')
    assert get_date_picker_value(page, "#filter-PatientBirthDate").startswith("20360203")
    expect(get_collapsed_studies(page)).to_have_count(0, timeout=2000)
