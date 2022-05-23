<script>

import UploadHandler from "./UploadHandler.vue"
import JobsList from "./JobsList.vue";
import { mapState } from "vuex"
import { orthancApiUrl, oe2ApiUrl } from "../globalConfigurations";


export default {
    props: [],
    emits: [],
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
            system: state => state.configuration.system,
            queryableDicomModalities: state => state.configuration.queryableDicomModalities,
            queryableDicomWebServers: state => state.configuration.queryableDicomWebServers,
            studiesIds: state => state.studies.studiesIds,
            statistics: state => state.studies.statistics,
            jobs: state => state.jobs.jobsIds,
        }),

        hasQueryableDicomWebServers() {
            return false; // TODO this.queryableDicomWebServers.length > 0;
        },
        hasQueryableDicomModalities() {
            return false; // TODO this.queryableDicomModalities.length > 0;
        },
        hasAccessToSettings() {
            return this.uiOptions.EnableSettings;
        },
        hasJobs() {
            return this.jobs.length > 0;
        },
        displayedStudyCount() {
            return this.studiesIds.length;
        },
        orthancApiUrl() {
            return orthancApiUrl;
        }
    },
    components: { UploadHandler, JobsList }
}
</script>
<template>
    <div>
        <div>
            <img class="orthanc-logo" src="..//assets/images/orthanc.png" height="48" />
        </div>
        <div v-if="uiOptions.ShowOrthancName" class="orthanc-name">
            <p>{{ system.Name }}</p>
        </div>
        <div class="menu-list">
            <ul id="menu-content" class="menu-content collapse out">
                <li class="d-flex align-items-center">
                    <router-link class="router-link" to="/">
                        <i class="fa fa-x-ray fa-lg menu-icon"></i>Studies
                        <span class="ms-auto">{{ displayedStudyCount }} / {{ statistics.CountStudies }}</span>
                    </router-link>
                </li>

                <li v-if="uiOptions.EnableUpload" class="d-flex align-items-center" data-bs-toggle="collapse"
                    data-bs-target="#upload-handler">
                    <i class="fa fa-file-upload fa-lg menu-icon"></i>Upload
                    <span class="ms-auto"></span>
                </li>
                <div v-if="uiOptions.EnableUpload" class="collapse" id="upload-handler">
                    <UploadHandler />
                </div>

                <li v-if="hasQueryableDicomModalities" class="d-flex align-items-center" data-bs-toggle="collapse"
                    data-bs-target="#modalities-list">
                    <i class="fa fa-radiation fa-lg menu-icon"></i>DICOM Modalities
                    <span class="arrow ms-auto"></span>
                </li>
                <ul class="sub-menu collapse" id="modalities-list">
                    <li v-for="modality in queryableDicomModalities" :key="modality" class="active">
                        <a href="#">{{ modality }} (TODO)</a>
                    </li>
                </ul>

                <li v-if="hasQueryableDicomWebServers" class="d-flex align-items-center" data-bs-toggle="collapse"
                    data-bs-target="#dicomweb-servers-list">
                    <i class="fa fa-globe fa-lg menu-icon"></i>DICOM-Web Servers
                    <span class="arrow ms-auto"></span>
                </li>
                <ul class="sub-menu collapse" id="dicomweb-servers-list">
                    <li v-for="server in queryableDicomWebServers" :key="server" class="active">
                        <a href="#">{{ server }} (TODO)</a>
                    </li>
                </ul>

                <li v-if="hasAccessToSettings" class="d-flex align-items-center">
                    <router-link class="router-link" to="/settings">
                        <i class="fa fa-cogs fa-lg menu-icon"></i>Settings
                    </router-link>
                </li>

                <li v-if="uiOptions.EnableLinkToLegacyUi" class="d-flex align-items-center router-link">
                    <a v-bind:href="this.orthancApiUrl + 'app/explorer.html'">
                        <i class="fa fa-solid fa-backward fa-lg menu-icon"></i>Legacy UI
                    </a><span class="ms-auto"></span>
                </li>
                <li v-if="hasJobs" class="d-flex align-items-center router-link">
                    <a href="#">
                        <i class="fa fa-solid fa-bars-progress fa-lg menu-icon"></i>My jobs
                    </a><span class="ms-auto"></span>
                </li>
                <div v-if="hasJobs" class="collapse show" id="jobs-list">
                    <JobsList />
                </div>

            </ul>
        </div>
    </div>
</template>
<style scoped>
.router-link {
    margin-left: -10px;
    width: 100%;
    text-align: left;
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
}
</style>