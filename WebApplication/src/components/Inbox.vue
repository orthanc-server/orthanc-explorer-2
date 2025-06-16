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
            options: [],
        };
    },
    mounted() {
    },
    methods: {
        async onUploadCompleted(uploadedStudiesIds) {
            console.log("upload complete: ", uploadedStudiesIds);
            const response = await api.commitInbox('../../plugins/inbox-commit', uploadedStudiesIds, {"toto": "tutu"});
            console.log(response);
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
        <div class="row w-75 px-3 text-center">
        <UploadHandler @uploadCompleted="onUploadCompleted"/>
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