<script>
import { mapState } from "vuex"
import api from "../orthancApi"
import { translateDicomTag } from "../locales/i18n"
import dateHelpers from "../helpers/date-helpers"
import resourceHelpers from "../helpers/resource-helpers"
import UploadHandler from "./UploadHandler.vue"

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
    },
    setup() {
        return {
        }
    },

    data() {
        return {
            inboxConfig_: null,
            formValues: {},
            formFieldsValidities: {},
            formFieldsErrorMessages: {},
            formIsValid: false,
            isFormValuesInitialized: false,
            formValidityCheckerHandler: null,
            commitResponse: null,
            processingPctProcessed: 0,
            processingRefreshTimeout: 200,
            processingMessage: null,
            processingIsComplete: false,
            processingHasFailed: false,
        };
    },
    mounted() {
    },
    watch: {
        formValues: {
            handler(newValue, oldValue) {
                if (!this.isFormValuesInitialized) {
                    return;
                }
                
                // only trigger form validation when we stop typing for 300ms
                if (this.formValidityCheckerHandler) {
                    clearTimeout(this.formValidityCheckerHandler);
                }
                this.formValidityCheckerHandler = setTimeout(() => { this.updateFormValidity() }, 300);
            },
            deep: true
        },
        userProfile(newValue, oldValue) {
            // the user profile arrives after the inbox initialization

            for (let formField of this.inboxConfig_['FormFields']) {
                this.formValues[formField.Id] = null;

                // set the initial value to the only value
                if (formField.Type == 'UserGroupsChoice' && !this.hasMultipleUserGroupsChoices(formField)) {
                    this.formValues[formField.Id] = this.getUserGroupsChoices(formField)[0];
                }
            }
        }
    },
    methods: {
        async onUploadCompleted(uploadedStudiesIds) {
            // console.log("upload complete: ", uploadedStudiesIds, this.formValues);
            this.commitResponse = await api.commitInbox(this.inboxConfig['CommitUrl'], [...uploadedStudiesIds], this.formValues);
            this.startMonitoringProcessing();
        },
        async updateFormValidity() {
            let allValid = true;

            const response = await api.validateInboxForm(this.inboxConfig['FormValidationUrl'], this.formValues);

            for (const formField of this.formFields) {
                if (formField.Id in response) {
                    this.formFieldsValidities[formField.Id] = response[formField.Id]['IsValid']
                    if ('Message' in response[formField.Id]) {
                        this.formFieldsErrorMessages[formField.Id] = response[formField.Id]['Message'];
                    }
                    allValid &= this.formFieldsValidities[formField.Id]
                } else {
                    allValid = false;
                    console.error("No validation result for field '" + formField.Id + "'");
                }
            }
            this.formIsValid = allValid;
            console.log("canUpload", this.canUpload);
        },
        isFieldValid(formField) {
            return this.formFieldsValidities[formField.Id];
        },
        fieldErrorMessage(formField) {
            return this.formFieldsErrorMessages[formField.Id];
        },
        hasMultipleUserGroupsChoices(formField) {
            if (!this.userProfile) {
                return false;
            }
            const userGroupsChoices = this.getUserGroupsChoices(formField);
            return userGroupsChoices.length > 1;
        },
        getUserGroupsChoices(formField) {
            if (!this.userProfile) {
                return [];
            }

            const userGroups = this.userProfile.groups;
            if (!userGroups || userGroups.length == 0) {
                return [];
            }

            if ('GroupsRegex' in formField && formField['GroupsRegex']) {
                const groupsRegex = formField['GroupsRegex'];
                let matchingGroups = [];
                for (const userGroup of userGroups) {
                    const match = userGroup.match(groupsRegex);
                    if (match) {
                        matchingGroups.push(match[1]);
                    }
                }
                return matchingGroups;
            } else {
                return userGroups;
            }
        },
        startMonitoringProcessing() {
            this.processingRefreshTimeout = 200;  // refresh quickly at the beginnning !
            setTimeout(this.monitorProcessing, this.processingRefreshTimeout);
        },
        async monitorProcessing() {
            const processingStatus = await api.monitorInboxProcessing(this.inboxConfig['ProcessingMonitoringUrl'], this.commitResponse);

            this.processingIsComplete = processingStatus['IsComplete'];
            this.processingHasFailed = processingStatus['HasFailed'];
            this.processingPctProcessed = processingStatus['PctProcessed'];
            this.processingMessage = processingStatus['Message'];

            if (!this.processingIsComplete) {
                this.processingRefreshTimeout = Math.min(this.processingRefreshTimeout + 200, 2000);  // refresh quickly at the beginnning !
                setTimeout(this.monitorProcessing, this.processingRefreshTimeout);
            } else {

            }
        },
        reload() {
            window.location.reload()
        }

    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
            userProfile: state => state.configuration.userProfile,
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
        inboxConfig() {
            if (!this.inboxConfig_ && this.$appConfig && 'Inbox' in this.$appConfig && this.$appConfig['Inbox']) {
                this.inboxConfig_ = this.$appConfig['Inbox'];
                for (let formField of this.inboxConfig_['FormFields']) {
                    this.formValues[formField.Id] = null;
                }
                this.isFormValuesInitialized = true;
            }
            
            return this.inboxConfig_;
        },
        hasForm() {
            return this.inboxConfig['FormFields'].length > 0;
        },
        formFields() {
            return this.inboxConfig['FormFields'];
        },
        canUpload() {
            return !this.hasForm || this.formIsValid;
        },
        hasCustomTitle() {
            return 'Title' in this.inboxConfig;
        },
        customTitle() {
            return this.inboxConfig['Title'];
        },
        hasCustomIntroText() {
            return "IntroTextHtml" in this.inboxConfig;
        },
        customIntroText() {
            return this.inboxConfig['IntroTextHtml'];
        },
        isProcessing() {
            return this.commitResponse;
        },
        processingMessageComputed() {
            return this.processingMessage || this.commitResponse.Message;
        },
        processingProgress() {
            return this.processingPctComplete;
        }

    },
    components: {UploadHandler}
}
</script>

<template>
    <div class="d-flex flex-column min-vh-100 justify-content-center align-items-center">
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
            <div v-if="hasCustomTitle || uiOptions.ShowOrthancName" class="orthanc-name">
                <p><span v-if="hasCustomTitle">{{ customTitle }}</span>
                <span v-else-if="uiOptions.ShowOrthancName">{{ system.Name }}</span></p>
            </div>
        </div>
        <div class="row w-75 px-3">
            <p v-if="hasCustomIntroText" v-html="customIntroText"></p>
            <p v-else v-html="$t('inbox.generic_intro_text')"></p>
        </div>
        <div v-if="hasForm" class="row w-100 px-3">
            <div class="row w-100 py-1" v-for="formField in formFields" :key="formField.Id">
                <div class="col-6 tag-label text-end" >
                    <span>{{ formField.Title }}</span>
                </div>
                <div class="col-6 tag-value">
                    <input v-if="formField.Type=='Text'" v-model="formValues[formField.Id]" type="text" :placeholder="formField.Placeholder"/>
                    <select v-if="formField.Type=='Choice'" v-model="formValues[formField.Id]" class="form-select-sm">
                        <option v-if="formField.Placeholder" selected value="null">{{ formField.Placeholder }}</option>
                        <option v-for="choice of formField.Choices" :key="choice" :value="choice">{{ choice }}</option>
                    </select>
                    <select v-if="formField.Type=='UserGroupsChoice' && hasMultipleUserGroupsChoices(formField)" v-model="formValues[formField.Id]" class="form-select-sm">
                        <option v-if="formField.Placeholder" selected value="null">{{ formField.Placeholder }}</option>
                        <option v-for="group of getUserGroupsChoices(formField)" :key="group" :value="group">{{ group }}</option>
                    </select>
                    <select v-if="formField.Type=='UserGroupsChoice' && !hasMultipleUserGroupsChoices(formField)" v-model="formValues[formField.Id]" class="form-select-sm" disabled>
                        <option selected :value="getUserGroupsChoices(formField)[0]">{{ getUserGroupsChoices(formField)[0] }}</option>
                    </select>
                    <span class="mx-1 text-success" v-if="isFieldValid(formField)"><i class="bi-check"></i></span>
                    <span class="mx-1 text-danger" v-if="!isFieldValid(formField)"><i class="bi-x"></i>{{ fieldErrorMessage(formField) }}</span>
                </div>
            </div>
        </div>
        <div class="row w-75 mt-2 px-3 text-center">
            <UploadHandler @uploadCompleted="onUploadCompleted" :showStudyDetails="false" :uploadDisabled="!canUpload" :singleUse="true" :disableCloseReport="true" :uploadDisabledMessage="$t('inbox.fill_form_first')"/>
        </div>
        <div v-if="isProcessing" class="row w-75 mt-2 px-3 text-center my-2">
            <div>
            <div class="card border-secondary job-card">
                <div class="card-header jobs-header">
                    <p v-if="isProcessing">{{ processingMessageComputed }}</p>
                    <div class="progress mt-1 mb-1" style="width:90%">
                        <div class="progress-bar" :class="{'bg-failure': processingHasFailed, 'bg-success': processingIsComplete && (processingPctProcessed >= 100), 'bg-secondary': !processingHasFailed &&!processingIsComplete}" role="progressbar"
                            v-bind:style="'width: ' + this.processingPctProcessed + '%'"></div>
                    </div>
                </div>
                </div>
            </div>
        </div>
        <div v-if="processingIsComplete" class="row mt-2 px-3 text-center my-2">
            <button type="button" class="btn btn-secondary" @click="reload"><i class="bi bi-arrow-clockwise mx-1"></i>{{ $t('inbox.reload_inbox') }}</button>
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
</style>