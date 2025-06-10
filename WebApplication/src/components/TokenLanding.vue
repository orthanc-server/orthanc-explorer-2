<script>
import { mapState } from "vuex"
import api from "../orthancApi"
import { translateDicomTag } from "../locales/i18n"
import dateHelpers from "../helpers/date-helpers"
import resourceHelpers from "../helpers/resource-helpers"


export default {
    props: [],
    async created() {
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
        }
    },
    computed: {
        ...mapState({
            tokens: state => state.configuration.tokens,
            uiOptions: state => state.configuration.uiOptions,
        }),
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
    components: {}
}
</script>

<template>
    <div class="d-flex flex-column min-vh-100 justify-content-center align-items-center">
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
                <button v-if="option.Type == 'custom'" type="button" class="btn btn-primary"
                    @click="customButtonClicked(option)">
                    <span><i :class="customButtonIcon(option) + ' px-1'"/> {{ option.Title }}</span>
                </button>
            </p>
        </div>
    </div>
</template>

<style>
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