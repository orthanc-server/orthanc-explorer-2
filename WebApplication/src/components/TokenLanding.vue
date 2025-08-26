<script>
import { mapState } from "vuex"
import api from "../orthancApi"
import { translateDicomTag } from "../locales/i18n"
import dateHelpers from "../helpers/date-helpers"
import resourceHelpers from "../helpers/resource-helpers"
import LanguagePicker from "./LanguagePicker.vue";

function applyBootStrapTheme() {
    // hack to switch the theme: get the value from our custom css
    let bootstrapTheme = getComputedStyle(document.documentElement).getPropertyValue('--bootstrap-theme');
    console.log("-------------- Applying Bootstrap theme ...", bootstrapTheme);
    if (bootstrapTheme) {
        // and set it to the 'html' element
        document.documentElement.setAttribute('data-bs-theme', bootstrapTheme);
    } else {
        console.warn("-------------- Applying Bootstrap theme not defined yet, retrying ...");
        setTimeout(applyBootStrapTheme, 100);
    }
}


export default {
    props: [],
    async created() {
        {// Load the CSS dynamically since it can be a custom css
            console.log("Loading the defaults + custom CSS ...");

            let link = document.createElement('link');
            link.rel = 'stylesheet';
            link.type = 'text/css';
            link.href = 'customizable/custom.css';
    
            document.getElementsByTagName('HEAD')[0].appendChild(link);

            setTimeout(applyBootStrapTheme, 0);
        }


        const params = new URLSearchParams(window.location.search);
        this.token = params.get('token');
        const decodedToken = await api.parseToken('token', params.get('token'));
        this.tokenChecked = true;

        if ('ErrorCode' in decodedToken) {
            this.errorCode = decodedToken['ErrorCode'];
        } else if (this.$appConfig && 'TokensLandingOptions' in this.$appConfig && this.$appConfig['TokensLandingOptions'] && this.$appConfig['TokensLandingOptions'].length > 0) {
            this.resourcesOrthancIds = decodedToken['ResourcesOrthancIds'];
            this.resourcesDicomIds = decodedToken['ResourcesDicomIds'];
            this.options = this.$appConfig['TokensLandingOptions'];
            this.redirectUrl = decodedToken['RedirectUrl'];
            
            if (this.resourcesOrthancIds.length == 1) {
                let study = await api.getStudy(this.resourcesOrthancIds[0]);
                this.studyMainDicomTags = {...study['MainDicomTags'], ...study['PatientMainDicomTags']};
            }
        } else if ('RedirectUrl' in decodedToken) {
            // There are no options to provide to the user -> redirect to the viewer.
            // This is the default behaviour.
            window.location = decodedToken['RedirectUrl'];
        }
    },
    setup() {
        return {
        }
    },

    data() {
        return {
            tokenChecked: false,
            token: null,
            redirectUrl: null,
            errorCode: null,
            options: [],
            resourcesOrthancIds: [],
            resourcesDicomIds: [],
            studyMainDicomTags: {},
            tagsToDisplay: ['PatientID', 'PatientName', 'PatientBirthDate', 'PatientSex', 'StudyDate', 'StudyDescription']
        };
    },
    mounted() {
    },
    methods: {
        titleForTag(tag) {
            return translateDicomTag(this.$i18n.t, this.$i18n.te, tag);
        },
        valueForTag(tag) {
            if (tag in this.studyMainDicomTags) {
                let value = this.studyMainDicomTags[tag];
                if (dateHelpers.isDateTag(tag)) {
                    return dateHelpers.formatDateForDisplay(value, this.uiOptions.DateFormat)
                } else {
                    return value;
                }
            } else {
                return "";
            }
        },
        viewStudy() {
            window.open(this.redirectUrl, '_blank');
        },
        _clickHiddenLink(url) {
            let finalUrl = url;

            if (finalUrl.indexOf('?') == -1) {
                finalUrl = finalUrl + "?token=" + this.token;
            } else {
                finalUrl = finalUrl + "&token=" + this.token;
            }
            console.log(finalUrl);

            let link = document.querySelector("#hidden-link-for-tokens");
            if (link == null) {
                link = document.createElement('a');
                document.body.appendChild(link);
                link.setAttribute("id", "hidden-link-for-tokens");
                link.setAttribute("type", "hidden");
            }
            link.target = "_self";
            link.href = finalUrl;
            link.click();
        },
        downloadStudy() {
            this._clickHiddenLink(this.downloadZipUrl);
        },
        customButtonIcon(option) {
            return option.Icon;
        },
        customButtonClicked(option) {
            let url = resourceHelpers.replaceResourceTagsInStringPlainText(option.Url, this.studyMainDicomTags, null, null, null, this.resourcesOrthancIds[0], 'study');
            url = url.replace("{token}", this.token);

            this._clickHiddenLink(url);
        },
        isCustomButtonEnabled(option) {
            return this.resourcesOrthancIds.length == 1 || ('EnableForMultipleResources' in option && option['EnableForMultipleResources']);
        }
    },
    computed: {
        ...mapState({
            tokens: state => state.configuration.tokens,
            uiOptions: state => state.configuration.uiOptions,
            system: state => state.configuration.system,
            hasCustomLogo: state => state.configuration.hasCustomLogo,
            configuration: state => state.configuration,
        }),
        customLogoUrl() {
            if (this.hasCustomLogo && this.configuration.customLogoUrl) {
                return this.configuration.customLogoUrl;
            } else {
                return "./customizable/custom-logo";
            }
        },
        showOptions() {
            return this.options.length > 0;
        },
        showStudyDetails() {
            return this.showOptions && this.resourcesOrthancIds.length == 1;
        },
        hasDownloadStudyButton() {
            return true;
        },
        hasOpenInViewerButton() {
            return true;
        },
        downloadZipUrl() {
            if (this.resourcesOrthancIds.length != 1) {
                return api.getBulkDownloadZipUrl(this.resourcesOrthancIds);
            }
            let filename = resourceHelpers.replaceResourceTagsInStringPlainText(this.uiOptions.DownloadStudyFileNameTemplate, this.studyMainDicomTags, null, null, null, this.resourcesOrthancIds[0], 'study');
            
            return api.getDownloadZipUrl('study', this.resourcesOrthancIds[0], filename);
        },
    },
    components: { LanguagePicker }
}
</script>

<template>
    <div class="d-flex flex-column min-vh-100">
        <div class="row w-100 mt-2">
            <div class="col-10"></div>
            <div class="col-2"><LanguagePicker></LanguagePicker></div>
        </div>
        <div class="d-flex flex-column justify-content-center align-items-center mt-5">
            <div class="row w-100 px-3 h4 text-center">
                <div v-if="!hasCustomLogo">
                    <img class="orthanc-logo" src="../assets/images/orthanc.png"/>
                </div>
                <div v-if="hasCustomLogo">
                    <img class="custom-logo" :src="customLogoUrl" />
                </div>
                <div v-if="hasCustomLogo">
                    <p class="powered-by-orthanc">
                    powered by
                    <img src="../assets/images/orthanc.png" />
                    </p>
                </div>
                <div v-if="uiOptions.ShowOrthancName" class="orthanc-name">
                    <p>{{ system.Name }}</p>
                </div>
            </div>
            <div class="row w-100 px-3 h4 text-center">
                <p v-if="tokenChecked && showOptions && showStudyDetails" v-html="$t('token.landing_options_single_study_intro_text')"></p>
                <p v-if="tokenChecked && showOptions && !showStudyDetails" v-html="$t('token.landing_options_multiple_studies_intro_text', {count: this.resourcesOrthancIds.length})"></p>
                <span v-if="!tokenChecked">
                    <p v-html="$t('token.token_being_checked_html')"></p>
                    <div class="spinner-border" role="status">
                        <span class="visually-hidden">Loading...</span>
                    </div>
                </span>
                <p v-if="tokenChecked && errorCode == 'invalid'" v-html="$t('token.error_token_invalid_html')"></p>
                <p v-if="tokenChecked && errorCode == 'expired'" v-html="$t('token.error_token_expired_html')"></p>
                <p v-if="tokenChecked && errorCode == 'unknown'" v-html="$t('token.error_token_unknown_html')"></p>
            </div>
            <div v-if="tokenChecked && showOptions" class="w-100">
                <div v-if="showStudyDetails" class="w-100">
                    <div class="row w-100 px-3" v-for="tag in tagsToDisplay" :key="tag">
                        <div class="col-6 tag-label" >
                            <span v-html="titleForTag(tag)"></span>
                        </div>
                        <div class="col-6 tag-value">
                            <span v-html="valueForTag(tag)"></span>
                        </div>
                    </div>
                </div>
                <div class="row w-100 m-3"></div> 
                <p v-for="option in options" :key="option" class="text-center">
                    <button v-if="option.Type == 'download-study'" type="button" class="btn btn-primary"
                        @click="downloadStudy()">
                        <span v-if="showStudyDetails"><i class="bi bi-download px-1"/> {{ $t('token.download_study') }}</span>
                        <span v-if="!showStudyDetails"><i class="bi bi-download px-1"/> {{ $t('token.download_studies', {count: this.resourcesOrthancIds.length}) }}</span>
                    </button>
                    <button v-if="option.Type == 'open-viewer-button'" type="button" class="btn btn-primary"
                        @click="viewStudy()">
                        <span v-if="showStudyDetails"><i class="bi bi-eye px-1"/> {{ $t('token.open_study_in_viewer') }}</span>
                        <span v-if="!showStudyDetails"><i class="bi bi-eye px-1"/> {{ $t('token.open_studies_in_viewer', {count: this.resourcesOrthancIds.length}) }}</span>
                    </button>
                    <button v-if="option.Type == 'custom' && isCustomButtonEnabled(option)" type="button" class="btn btn-primary"
                        @click="customButtonClicked(option)">
                        <span><i :class="customButtonIcon(option) + ' px-1'"/> {{ option.Title }}</span>
                    </button>
                </p>
            </div>
        </div>
    </div>
</template>

<style>

body {
    background-color: var(--nav-side-bg-color) !important;
    color: var(--nav-side-submenu-color) !important;
}

.orthanc-name {
    border-bottom-width: 1px;
    border-bottom-style: solid;
    font-size: 1rem;
    margin-bottom: 3em;
}

.orthanc-name p {
    margin-bottom: 1.5rem;
    font-weight: 500;
    font-size: x-large;
}

.orthanc-logo {
    filter: brightness(50);
    height: 100px;
}

.powered-by-orthanc {
    font-size: small;
}

.powered-by-orthanc > img {
    filter: brightness(50);
    max-width: 50%;
    max-height: 30px;
}

.custom-logo {
    padding: 4px;
    max-width: 90%;
    max-height: 150px;
}

.tag-label {
    font-weight: 700;
    max-width: 50vw;
    overflow: hidden;
    text-overflow: ellipsis;
    text-align: right;
}

.tag-value {
    margin-left: auto !important;
    font-weight: 500;
    max-width: 50vw;
    overflow: hidden;
    text-overflow: ellipsis;
}
</style>