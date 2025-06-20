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
            formValidityCheckerHandler: null
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
    },
    methods: {
        async onUploadCompleted(uploadedStudiesIds) {
            console.log("upload complete: ", uploadedStudiesIds, this.formValues);
            const response = await api.commitInbox(this.inboxConfig['CommitUrl'], [...uploadedStudiesIds], this.formValues);
            console.log(response);
        },
        updateFormValidity() {
            let allValid = true;

            for (let formField of this.formFields) {
                if ('Mandatory' in formField && formField['Mandatory']) {
                    if (formField.Type == "text") {
                        this.formFieldsValidities[formField.Id] = (this.formValues[formField.Id] && (this.formValues[formField.Id] != ""));
                        allValid = allValid & this.formFieldsValidities[formField.Id];
                        if (!this.formFieldsValidities[formField.Id]) {
                            this.formFieldsErrorMessages[formField.Id] = "non";
                        } else {
                            this.formFieldsErrorMessages[formField.Id] = null;
                        }
                        console.log("formIsValid", allValid);
                    } 
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
        }
    },
    computed: {
        ...mapState({
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
            <div v-if="uiOptions.ShowOrthancName" class="orthanc-name">
                <p>{{ system.Name }}</p>
            </div>
        </div>
        <div class="row w-100 px-3 h4 text-center">
            <p v-html="$t('inbox.generic_intro_text')"></p>
        </div>
        <div v-if="hasForm" class="row w-75 px-3">
            <div class="row w-100 py-1" v-for="formField in formFields" :key="formField.Id">
                <div class="col-6 tag-label text-end" >
                    <span>{{ formField.Title }}</span>
                </div>
                <div class="col-6 tag-value">
                    <input v-if="formField.Type=='text'" v-model="formValues[formField.Id]" type="text" :placeholder="formField.Placeholder"/>
                    <span class="mx-1 text-success" v-if="isFieldValid(formField)"><i class="bi-check"></i></span>
                    <span class="mx-1 text-danger" v-if="!isFieldValid(formField)"><i class="bi-x"></i>{{ fieldErrorMessage(formField) }}</span>
                </div>
            </div>

        </div>
        <div class="row w-75 mt-2 px-3 text-center">
            <UploadHandler @uploadCompleted="onUploadCompleted" :showStudyDetails="false" :uploadDisabled="!canUpload" :uploadDisabledMessage="$t('inbox.fill_form_first')"/>
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