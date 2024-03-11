<script>

import UploadHandler from "./UploadHandler.vue"
import JobsList from "./JobsList.vue";
import LanguagePicker from "./LanguagePicker.vue";
import { mapState } from "vuex"
import { orthancApiUrl, oe2ApiUrl } from "../globalConfigurations";
import api from "../orthancApi"

export default {
    props: [],
    emits: [],
    data() {
        return {
            selectedModality: null,
            selectedLabel: null,
            modalitiesEchoStatus: {}
        };
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
            userProfile: state => state.configuration.userProfile,
            system: state => state.configuration.system,
            queryableDicomModalities: state => state.configuration.queryableDicomModalities,
            queryableDicomWebServers: state => state.configuration.queryableDicomWebServers,
            studiesIds: state => state.studies.studiesIds,
            statistics: state => state.studies.statistics,
            labelsFilter: state => state.studies.labelsFilter,
            jobs: state => state.jobs.jobsIds,
            allLabels: state => state.labels.allLabels,
            hasCustomLogo: state => state.configuration.hasCustomLogo,
            configuration: state => state.configuration,
        }),
        customLogoUrl() {
            if (this.hasCustomLogo && this.configuration.customLogoUrl) {
                return this.customLogoUrl;
            } else {
                return "./customizable/custom-logo";
            }
        },
        hasQueryableDicomWebServers() {
            return false; // TODO this.queryableDicomWebServers.length > 0;
        },
        hasQueryableDicomModalities() {
            return this.uiOptions.EnableDicomModalities && this.queryableDicomModalities.length > 0;
        },
        hasAccessToSettings() {
            return this.uiOptions.EnableSettings;
        },
        hasJobs() {
            return this.jobs.length > 0;
        },
        hasLogout() {
            return window.keycloak !== undefined;
        },
        hasUserProfile() {
            return this.userProfile != null && this.userProfile.name;
        },
        displayedStudyCount() {
            return this.studiesIds.length;
        },
        orthancApiUrl() {
            return orthancApiUrl;
        },
    },
    methods: {
        selectModality(modality) {
            this.selectedModality = modality;
        },
        isSelectedModality(modality) {
            return this.selectedModality === modality;
        },
        isEchoRunning(modality) {
            return this.modalitiesEchoStatus[modality] == null;
        },
        isEchoSuccess(modality) {
            return this.modalitiesEchoStatus[modality] == true;
        },
        selectLabel(label) {
            this.selectedLabel = label;
            this.messageBus.emit('filter-label-changed', label);
        },
        isSelectedLabel(label) {
            return this.labelsFilter.includes(label);
        },
        onAllLabelsChanged() {
            this.$store.dispatch('labels/refresh');
        },
        logout(event) {
            event.preventDefault();
            let logoutOptions = {
                "redirectUri": window.location.href
            }
            window.keycloak.logout(logoutOptions).then((success) => {
                console.log("logout success", success);
            }).catch((error) => {
                console.error("logout failed", error);
            })
        },
        changePassword(event) {
            event.preventDefault();
            window.keycloak.login({ action: "UPDATE_PASSWORD" }).then((success) => {
                console.log("login for password change success", success);
            }).catch((error) => {
                console.error("login for password change failed", error);
            })
        }

    },
    mounted() {
        this.$refs['modalities-collapsible'].addEventListener('show.bs.collapse', (e) => {
            for (const modality of this.queryableDicomModalities) {
                this.modalitiesEchoStatus[modality] = null;
            }
            for (const modality of this.queryableDicomModalities) {
                api.remoteModalityEcho(modality).then((response) => {
                    this.modalitiesEchoStatus[modality] = true;
                }).catch(() => {
                    this.modalitiesEchoStatus[modality] = false;
                })
            }
        });
    },
    components: { UploadHandler, JobsList, LanguagePicker },
}
</script>
<template>
    <div class="nav-side-menu">
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
        <div class="menu-list">
            <ul id="menu-content" class="menu-content collapse out">
                <li class="d-flex align-items-center fix-router-link">
                    <router-link class="router-link" to="/">
                        <i class="fa fa-x-ray fa-lg menu-icon"></i>{{ $t('local_studies') }}
                        <span class="study-count ms-auto">{{ displayedStudyCount }} / {{ statistics.CountStudies
                        }}</span>
                    </router-link>
                </li>
                <ul v-if="allLabels.length > 0" class="sub-menu" id="labels-list">
                    <li v-for="label in allLabels" :key="label"
                    v-bind:class="{ 'active': isSelectedLabel(label) }" @click="selectLabel(label)">
                        <i class="fa fa-tag label-icon"></i>
                        {{ label }}
                    </li>
                </ul>

                <li v-if="uiOptions.EnableUpload" class="d-flex align-items-center" data-bs-toggle="collapse"
                    data-bs-target="#upload-handler">
                    <i class="fa fa-file-upload fa-lg menu-icon"></i>{{ $t('upload') }}
                    <span class="ms-auto"></span>
                </li>
                <div v-if="uiOptions.EnableUpload" class="collapse" id="upload-handler">
                    <UploadHandler />
                </div>

                <li v-if="hasQueryableDicomModalities" class="d-flex align-items-center" data-bs-toggle="collapse"
                    data-bs-target="#modalities-list">
                    <i class="fa fa-radiation fa-lg menu-icon"></i>{{ $t('dicom_modalities') }}
                    <span class="arrow ms-auto"></span>
                </li>
                <ul class="sub-menu collapse" id="modalities-list" ref="modalities-collapsible">
                    <li v-for="modality in queryableDicomModalities" :key="modality"
                        v-bind:class="{ 'active': this.isSelectedModality(modality) }" @click="selectModality(modality)">
                        <router-link class="router-link"
                            :to="{ path: '/filtered-remote-studies', query: { remoteMode: 'dicom', remoteSource: modality } }">
                            {{ modality }}
                        </router-link>
                        <span v-if="this.isEchoRunning(modality)" class="ms-auto spinner-border spinner-border-sm"
                            data-bs-toggle="tooltip" title="Checking connectivity"></span>
                        <span v-else-if="this.isEchoSuccess(modality)" class="ms-auto"><i
                                class="bi bi-check2 text-success echo-status" data-bs-toggle="tooltip"
                                title="C-Echo succeeded"></i></span>
                        <span v-else class="ms-auto"><i class="bi bi-x-lg text-danger echo-status" data-bs-toggle="tooltip"
                                title="C-Echo failed"></i></span>
                    </li>
                </ul>

                <li v-if="hasQueryableDicomWebServers" class="d-flex align-items-center" data-bs-toggle="collapse"
                    data-bs-target="#dicomweb-servers-list">
                    <i class="fa fa-globe fa-lg menu-icon"></i>{{ $t('dicom_web_servers') }}
                    <span class="arrow ms-auto"></span>
                </li>
                <ul class="sub-menu collapse" id="dicomweb-servers-list">
                    <li v-for="server in queryableDicomWebServers" :key="server" class="active">
                        <a href="#">{{ server }} (TODO)</a>
                    </li>
                </ul>

                <li v-if="hasAccessToSettings" class="d-flex align-items-center fix-router-link">
                    <router-link class="router-link" to="/settings">
                        <i class="fa fa-cogs fa-lg menu-icon"></i>{{ $t('settings') }}
                    </router-link>
                </li>

                <li v-if="uiOptions.EnableLinkToLegacyUi" class="d-flex align-items-center fix-router-link">
                    <a v-bind:href="this.orthancApiUrl + 'app/explorer.html'">
                        <i class="fa fa-solid fa-backward fa-lg menu-icon"></i>{{ $t('legacy_ui') }}
                    </a><span class="ms-auto"></span>
                </li>
                <li v-if="hasLogout" class="d-flex align-items-center" data-bs-toggle="collapse"
                    data-bs-target="#profile-list">
                    <i class="fa fa-user fa-lg menu-icon"></i><span v-if="hasUserProfile">{{ userProfile.name }}</span><span v-if="!hasUserProfile">{{ $t('profile') }}</span>
                    <span class="arrow ms-auto"></span>
                </li>
                <ul class="sub-menu collapse" id="profile-list" ref="profile-collapsible">
                    <li v-if="uiOptions.EnableChangePassword" class="d-flex align-items-center fix-router-link">
                        <a v-bind:href="'#'" @click="changePassword($event)">
                            <i class="fa fa-solid fa-key fa-lg menu-icon"></i>{{ $t('change_password') }}
                        </a><span class="ms-auto"></span>
                    </li>
                    <li v-if="hasLogout" class="d-flex align-items-center fix-router-link">
                        <a v-bind:href="'#'" @click="logout($event)">
                            <i class="fa fa-solid fa-arrow-right-from-bracket fa-lg menu-icon"></i>{{ $t('logout') }}
                        </a><span class="ms-auto"></span>
                    </li>
                </ul>
                <li v-if="hasJobs" class="d-flex align-items-center">
                    <a href="#">
                        <i class="fa fa-solid fa-bars-progress fa-lg menu-icon"></i>{{ $t('my_jobs') }}
                    </a><span class="ms-auto"></span>
                </li>
                <div v-if="hasJobs" class="collapse show" id="jobs-list">
                    <JobsList />
                </div>
            </ul>
        </div>
        <div class="bottom-side-bar">
            <LanguagePicker />
        </div>
    </div>
</template>
<style scoped>
.router-link {
    width: 100%;
    text-align: left;
}

.fix-router-link {
    margin-left: -20px !important;
}

.echo-status {
    font-size: 17px;
}

.orthanc-name {
    border-bottom-width: 1px;
    border-bottom-style: solid;
    font-size: 1rem;
    margin-bottom: 0.3rem;
}

.orthanc-name p {
    margin-bottom: 0.3rem;
    font-weight: 500;
}

.orthanc-logo {
    filter: brightness(50);
    height: 48px;
}

.powered-by-orthanc > img {
    filter: brightness(50);
    max-width: 50%;
    height: auto;
}

.custom-logo {
    padding: 4px;
    max-width: 90%;
    height: auto;
}

.nav-side-menu {
    font-family: verdana;
    font-size: 12px;
    font-weight: 200;
    background-color: var(--nav-side-bg-color);
    color: var(--nav-side-color);
    height: 100%;
}


.nav-side-menu ul,
.nav-side-menu li {
    list-style: none;
    padding: 0px;
    margin: 0px;
    line-height: 35px;
    cursor: pointer;
}

.nav-side-menu ul :not(collapsed) .arrow:before,
.nav-side-menu li :not(collapsed) .arrow:before {
    font-family: "Font Awesome\ 5 Free";
    font-weight: 900;
    content: "\f0d7";
    display: inline-block;
    padding-left: 10px;
    padding-right: 10px;
    vertical-align: middle;
    float: right;
}

.nav-side-menu li .study-count {
    padding-right: 0px;
    float: right;
}

.nav-side-menu ul .active,
.nav-side-menu li .active {
    border-left: 3px solid var(--nav-side-active-border-color);
    background-color: var(--nav-side-selected-bg-color);
}

.nav-side-menu ul .sub-menu li.active,
.nav-side-menu li .sub-menu li.active {
    color: var(--nav-side-submenu-color);
    background-color: var(--nav-side-selected-bg-color);
}

.nav-side-menu ul .sub-menu li.active a,
.nav-side-menu li .sub-menu li.active a {
    color: var(--nav-side-submenu-color);
}

.nav-side-menu ul .sub-menu li,
.nav-side-menu li .sub-menu li {
    display: flex;
    background-color: var(--nav-side-sub-bg-color);
    border: none;
    line-height: 28px;
    border-bottom: 1px solid var(--nav-side-bg-color);
    margin-left: 0px;
}

.nav-side-menu ul .sub-menu li:hover,
.nav-side-menu li .sub-menu li:hover {
    border-left: 3px solid var(--nav-side-active-border-color);
    background-color: var(--nav-side-selected-bg-color);
}

.nav-side-menu ul .sub-menu li:before,
.nav-side-menu li .sub-menu li:before {
    font-family: "Font Awesome\ 5 Free";
    font-weight: 900;
    content: " ";
    display: inline-block;
    padding-left: 20px;
    padding-right: 20px;
    vertical-align: middle;
}

.nav-side-menu li {
    margin-left: -10px;
    padding-left: 0px;
    border-left: 3px solid var(--nav-side-bg-color);
    border-bottom: 1px solid var(--nav-side-bg-color);
}

.nav-side-menu li a {
    text-decoration: none;
    color: var(--nav-side-color);
}

.nav-side-menu li a i {
    padding-left: 10px;
    width: 20px;
    padding-right: 20px;
}

.nav-side-menu li:hover {
    border-left: 3px solid var(--nav-side-active-border-color);
    background-color: var(--nav-side-selected-bg-color);
}

.nav-side-menu .menu-list .menu-content {
    display: block;
}

.nav-side-menu .menu-list .menu-content {
    display: block;
}

.menu-list {
    margin-left: 10px;
    margin-right: 10px;
    font-size: 14px;
}

.menu-icon {
    width: 20px;
    margin-right: 10px;
}

.label-icon {
    width: 15px;
    margin-right: 5px;
    line-height: 28px;
}

.bottom-side-bar {
    position: absolute;
    bottom: 1rem;
    width: 100%;
}
</style>