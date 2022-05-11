<script>

import UploadHandler from "./UploadHandler.vue"
import { mapState } from "vuex"

export default {
    props: [],
    emits: [],
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
            queryableDicomModalities: state => state.configuration.queryableDicomModalities,
            queryableDicomWebServers: state => state.configuration.queryableDicomWebServers,
            studiesIds: state => state.studies.studiesIds,
            statistics: state => state.studies.statistics,
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
        displayedStudyCount() {
            return this.studiesIds.length;
        }
    },
    components: { UploadHandler }
}
</script>
<template>
    <div>
        <div class="brand">
            <img src="..//assets/images/orthanc.png" height="48" />
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
                    <a href="app/explorer.html">
                        <i class="fa fa-solid fa-backward fa-lg menu-icon"></i>Legacy UI
                    </a><span class="ms-auto"></span>
                </li>
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
</style>