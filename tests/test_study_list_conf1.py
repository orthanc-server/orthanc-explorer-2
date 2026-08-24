from playwright.sync_api import sync_playwright, Page, expect
from orthanc_api_client import OrthancApiClient
from helpers import *
import pytest
import pathlib

here = pathlib.Path(__file__).parent.resolve()



UI_CONFIG = "./configs/conf-1.json"

def test_search_study_list(page: Page, orthanc_api: OrthancApiClient):
    orthanc_api.delete_all_content()

    orthanc_api.upload_file(here / "stimuli/TEST_1/10.dcm")
    orthanc_api.upload_file(here / "stimuli/TEST_1/12.dcm")

    expect(page.locator("#filter-PatientName")).to_be_visible()

    page.locator("#filter-PatientName").fill("Te")
    expect(get_collapsed_studies(page)).to_have_count(1)

    page.locator("#filter-PatientName").fill("Arn")
    # page.screenshot(path=here / "screenshots/debug.png", full_page=True)
    expect(get_collapsed_studies(page)).to_have_count(0)


