from playwright.sync_api import sync_playwright, Page, expect
from orthanc_api_client import OrthancApiClient
from helpers import *
import pytest
import pathlib
import time
import re
from conftest import ScreenMode

here = pathlib.Path(__file__).parent.resolve()

CONFIG_NAME = "conf1"
COMPOSE_FILE = "docker-compose.no-auth.yml"
HAS_KEYCLOAK = False
HAS_NGINX = False

ORTHANC_CONFIG = {
    "AuthenticationEnabled": False,
    "OverwriteInstances": "IfChanged",

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

    wait_study_list_visible(page)

    fill_filter(page, "PatientName", "Tes")
    expect(get_collapsed_studies(page)).to_have_count(1, timeout=2000)

    fill_filter(page, "PatientName", "Arn")
    expect(get_collapsed_studies(page)).to_have_count(0, timeout=2000)

    # open study list from URL (with PatientName)
    page.goto('/ui/app/filtered-studies?PatientName=Test&order-by=Metadata,LastUpdate,DESC')
    expect_filter_to_have_value(page, "PatientName", "Test")
    expect(get_collapsed_studies(page)).to_have_count(1, timeout=2000)

    # open study list from URL (with PatientID)
    page.goto('/ui/app/filtered-studies?PatientID=TEST_1&order-by=Metadata,LastUpdate,DESC')
    expect_filter_to_have_value(page, "PatientID", "TEST_1")
    expect(get_collapsed_studies(page)).to_have_count(1, timeout=2000)

    # open study list from URL (no match)
    page.goto('/ui/app/filtered-studies?PatientBirthDate=20360203&order-by=Metadata,LastUpdate,DESC')
    expected_date_picker_to_have_value(page, "PatientBirthDate", re.compile(r"^20360203.*"))
    expect(get_collapsed_studies(page)).to_have_count(0, timeout=2000)


def test_modify_utf8(page: Page, orthanc_api: OrthancApiClient):
    orthanc_api.delete_all_content()

    uploaded_instances_ids = orthanc_api.upload_file(here / "stimuli/TEST_1/10.dcm")
    uploaded_study_id = orthanc_api.instances.get_parent_study_id(uploaded_instances_ids[0])
    wait_study_list_visible(page)
    expand_first_study(page)

    with TestContext("Modify a PatientName with UTF-8 characters while the initial file is in Latin1"):

        expect(page.locator(f"#modify-button-{uploaded_study_id}")).to_be_visible()
        expect(page.locator(f"#modify-modal-{uploaded_study_id}")).to_be_hidden()
        page.locator(f"#modify-button-{uploaded_study_id}").click()
        expect(page.locator(f"#modify-modal-{uploaded_study_id}")).to_be_visible()
        expect(page.locator("#modify-button-any-tags-in-one-study")).to_be_visible()
        page.locator("#modify-button-any-tags-in-one-study").click()

        expect(page.locator("#modify-footer-button-modify")).to_be_visible()
        expect(page.locator("#modify-footer-button-modify")).to_be_disabled()
        expect(page.locator("#modify-input-PatientID")).to_be_visible()
        page.locator("#modify-input-PatientName").fill("UTF-8林建和")
        page.locator(f'input[type="radio"][value="modify-keep-uids"]').click()

        expect(page.locator("#modify-footer-button-modify")).to_be_enabled()
        page.locator("#modify-footer-button-modify").click()

        expect(page.locator("#modify-button-show-modified-resources")).to_be_visible()
        expect(page.locator("#modify-button-show-modified-resources")).to_be_enabled()
        page.locator("#modify-button-show-modified-resources").click()

        expect(get_expanded_studies(page)).to_have_count(1, timeout=2000)
        assert "UTF-8林建和" in get_texts_from_first_collapsed_study(page)


@pytest.mark.parametrize("screen_mode", [ScreenMode.MOBILE_NARROW], indirect=True)
def test_mobile_narrow_smoke(page: Page, orthanc_api: OrthancApiClient, screen_mode: ScreenMode):
    orthanc_api.delete_all_content()

    orthanc_api.upload_file(here / "stimuli/TEST_1/10.dcm")
    orthanc_api.upload_file(here / "stimuli/TEST_1/12.dcm")

    with TestContext("study list is rendered as mobile cards, not as a desktop table"):
        take_screenshot(page, "mobile")
        wait_study_list_visible(page)
        expect(page.locator("table.study-table")).to_have_count(0)
        expect(page.locator(".mobile-study-list")).to_be_visible()
        expect(get_mobile_study_cards(page)).to_have_count(1)

    with TestContext("search-as-you-type works in the mobile filter strip"):
        fill_filter(page, "PatientName", "Tes")
        expect(get_mobile_study_cards(page)).to_have_count(1)

        fill_filter(page, "PatientName", "Arn")
        expect(get_mobile_study_cards(page)).to_have_count(0)

    # with TestContext("a study card can be expanded and collapsed"):
    #     expand_first_mobile_study_card(page)
    #     expect(page.locator(".mobile-resource-card.expanded")).to_have_count(1)
    #     # expanding a study card shows its series as mobile cards
    #     expect(page.locator(".mobile-series-card").first).to_be_visible()

    #     expand_first_mobile_study_card(page)
    #     expect(page.locator(".mobile-resource-card.expanded")).to_have_count(0)

    # with TestContext("the mobile sidebar opens and closes with the burger button"):
    #     toggle = page.locator("#mobile-sidebar-toggle")
    #     expect(toggle).to_be_visible()
    #     expect(toggle).to_have_attribute("aria-expanded", "false")

    #     toggle.click()
    #     expect(toggle).to_have_attribute("aria-expanded", "true")

    #     toggle.click()
    #     expect(toggle).to_have_attribute("aria-expanded", "false")



