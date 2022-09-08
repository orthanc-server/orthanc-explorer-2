<script>
import Modal from "./Modal.vue"
import $ from "jquery"
import { mapState } from "vuex"
import api from "../orthancApi"


export default {
    props: ["resourceOrthancId", "resourceDicomUid", "resourceLevel", "customClass"],
    setup() {
        return {
        }
    },

    data() {
        return {
            "isDeleteModalVisible": false,
        };
    },
    mounted() {
    },
    methods: {
        toggleSubMenu(event) {
            let parent = event.target.parentElement; // TODO: find a clean way to go to next ul 
            if (event.target.tagName == "I") {
                parent = parent.parentElement;
            }
            $(parent.querySelector(".dropdown-menu")).toggle();
            event.stopPropagation();
            event.preventDefault();
        },
        deleteResource(event) {
            api.deleteResource(this.resourceLevel, this.resourceOrthancId)
                .then(() => {
                    this.$emit("deletedResource");
                })
                .catch((reason) => {
                    console.error("failed to delete resource : ", this.resourceOrthancId, reason);
                });
        },
        getApiUrl(subRoute) {
            return api.getApiUrl(this.resourceLevel, this.resourceOrthancId, subRoute);
        },
        async sendToDicomWebServer(server) {
            const jobId = await api.sendToDicomWebServer([this.resourceOrthancId], server);
            this.$store.dispatch('jobs/addJob', { jobId: jobId, name: 'Send to DicomWeb (' + server + ')' });
        },
        async sendToOrthancPeer(peer) {
            const jobId = await api.sendToOrthancPeer([this.resourceOrthancId], peer);
            this.$store.dispatch('jobs/addJob', { jobId: jobId, name: 'Send to Peer (' + peer + ')' });
        },
        async sendToOrthancPeerWithTransfers(peer) {
            const jobId = await api.sendToOrthancPeerWithTransfers([{ "Level": this.capitalizeFirstLetter(this.resourceLevel), "ID": this.resourceOrthancId }], peer);
            this.$store.dispatch('jobs/addJob', { jobId: jobId, name: 'Transfer to Peer (' + peer + ')' });
        },
        async sendToDicomModality(modality) {
            const jobId = await api.sendToDicomModality([this.resourceOrthancId], modality);
            this.$store.dispatch('jobs/addJob', { jobId: jobId, name: 'Send to DICOM (' + modality + ')' });
        },
        capitalizeFirstLetter(level) {
            return level.charAt(0).toUpperCase() + level.slice(1);
        }
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
            installedPlugins: state => state.configuration.installedPlugins,
            targetDicomWebServers: state => state.configuration.targetDicomWebServers,
            targetDicomModalities: state => state.configuration.targetDicomModalities,
            orthancPeers: state => state.configuration.orthancPeers
        }),
        hasSendTo() {
            return this.uiOptions.EnableSendTo &&
                (this.hasSendToDicomWeb || this.hasSendToPeers || this.hasSendToDicomModalities || this.hasSendToPeersWithTransfer);
        },
        hasSendToPeers() {
            return this.orthancPeers.length > 0;
        },
        hasSendToPeersWithTransfer() {
            return this.hasSendToPeers && ("transfers" in this.installedPlugins);
        },
        hasSendToDicomWeb() {
            return this.targetDicomWebServers.length > 0;
        },
        hasSendToDicomModalities() {
            return this.targetDicomModalities.length > 0;
        },
        hasOsimisViewer() {
            return "osimis-web-viewer" in this.installedPlugins;
        },
        osimisViewerUrl() {
            return api.getOsimisViewerUrl(this.resourceLevel, this.resourceOrthancId);
        },
        hasStoneViewer() {
            return "stone-webviewer" in this.installedPlugins;
        },
        stoneViewerUrl() {
            return api.getStoneViewerUrl(this.resourceLevel, this.resourceDicomUid);
        },
        hasOhifViewer() {
            return this.uiOptions.EnableOpenInOhifViewer;
        },
        ohifViewerUrl() {
            return this.uiOptions.OhifViewerPublicRoot + this.resourceDicomUid;
        },
        hasMedDreamViewer() {
            return this.uiOptions.EnableOpenInMedDreamViewer;
        },
        medDreamViewerUrl() {
            return this.uiOptions.MedDreamViewerPublicRoot + "?study=" + this.resourceDicomUid;
        },
        instancePreviewUrl() {
            return api.getInstancePreviewUrl(this.resourceOrthancId);
        },
        instanceDownloadUrl() {
            return api.getInstanceDownloadUrl(this.resourceOrthancId);
        },
        downloadZipUrl() {
            return api.getDownloadZipUrl(this.resourceLevel, this.resourceOrthancId);
        },
        downloadDicomDirUrl() {
            return api.getDownloadDicomDirUrl(this.resourceLevel, this.resourceOrthancId);
        },
    },
    components: { Modal }
}
</script>

<template>
    <div>
        <div class="btn-group">
            <a v-if="hasOsimisViewer && (this.resourceLevel == 'study' || this.resourceLevel == 'series')"
                class="btn btn-sm btn-secondary m-1" type="button" data-bs-toggle="tooltip" title="View in OsimisViewer"
                target="blank" v-bind:href="osimisViewerUrl">
                <i class="bi bi-eye"></i>
            </a>
            <a v-if="hasStoneViewer && this.resourceLevel == 'study'" class="btn btn-sm btn-secondary m-1" type="button"
                data-bs-toggle="tooltip" title="View in StoneViewer" target="blank" v-bind:href="stoneViewerUrl">
                <i class="bi bi-eye-fill"></i>
            </a>
            <a v-if="hasOhifViewer && this.resourceLevel == 'study'" class="btn btn-sm btn-secondary m-1" type="button"
                data-bs-toggle="tooltip" title="View in OHIF" target="blank" v-bind:href="ohifViewerUrl">
                <i class="bi bi-grid"></i>
            </a>
            <a v-if="hasMedDreamViewer && this.resourceLevel == 'study'" class="btn btn-sm btn-secondary m-1"
                type="button" data-bs-toggle="tooltip" title="View in MedDream" target="blank"
                v-bind:href="medDreamViewerUrl">
                <i class="bi bi-columns-gap"></i>
            </a>
            <a v-if="this.resourceLevel == 'instance'" class="btn btn-sm btn-secondary m-1" type="button"
                data-bs-toggle="tooltip" title="Preview" target="blank" v-bind:href="instancePreviewUrl">
                <i class="bi bi-binoculars"></i>
            </a>
        </div>
        <div class="btn-group">
            <a v-if="uiOptions.EnableDownloadZip && this.resourceLevel != 'instance'"
                class="btn btn-sm btn-secondary m-1" type="button" data-bs-toggle="tooltip" title="Download ZIP"
                v-bind:href="downloadZipUrl">
                <i class="bi bi-download"></i>
            </a>
            <a v-if="uiOptions.EnableDownloadDicomDir && this.resourceLevel != 'instance'"
                class="btn btn-sm btn-secondary m-1" type="button" data-bs-toggle="tooltip" title="Download DICOMDIR"
                v-bind:href="downloadDicomDirUrl">
                <i class="bi bi-box-arrow-down"></i>
            </a>
            <a v-if="uiOptions.EnableDownloadDicomFile && this.resourceLevel == 'instance'"
                class="btn btn-sm btn-secondary m-1" type="button" data-bs-toggle="tooltip" title="Download DICOM file"
                v-bind:href="instanceDownloadUrl">
                <i class="bi bi-download"></i>
            </a>
        </div>
        <div class="btn-group">
            <!-- <button
            v-if="uiOptions.EnableAnonymize"
            class="btn btn-sm btn-secondary m-1"
            type="button"
            data-bs-toggle="tooltip"
            title="Anonymize (TODO)"
        >
            <i class="bi bi-person-x"></i>
        </button> -->
            <button v-if="uiOptions.EnableDeleteResources" class="btn btn-sm btn-secondary m-1" type="button"
                data-bs-toggle="modal" v-bind:data-bs-target="'#delete-modal-' + this.resourceOrthancId">
                <i class="bi bi-trash" data-bs-toggle="tooltip" title="Delete"></i>
            </button>
            <Modal v-if="uiOptions.EnableDeleteResources" :id="'delete-modal-' + this.resourceOrthancId"
                :headerText="'Delete ' + this.resourceLevel + ' ?'" :okText="'Delete'" :cancelText="'Cancel'"
                :bodyText="'Are you sure you want to delete this ' + this.resourceLevel + ' ?<br/>  This action can not be undone !'"
                @ok="deleteResource($event)"></Modal>
        </div>
        <div class="btn-group">
            <div class="dropdown">
                <button v-if="uiOptions.EnableApiViewMenu" class="dropdown btn btn-sm btn-secondary m-1 dropdown-toggle"
                    type="button" id="apiDropdownMenuId" data-bs-toggle="dropdown" aria-expanded="false">
                    <span data-bs-toggle="tooltip" title="API">
                        <i class="bi bi-code-slash"></i>
                    </span>
                </button>
                <ul class="dropdown-menu bg-dropdown" aria-labelledby="apiDropdownMenuId"
                    v-if="uiOptions.EnableApiViewMenu">
                    <li>
                        <button class="dropdown-item" href="#" v-clipboard:copy="this.resourceOrthancId">copy {{
                        this.resourceLevel }} orthanc id</button>
                    </li>
                    <li>
                        <a class="dropdown-item" target="blank" v-bind:href="getApiUrl('')">/</a>
                    </li>
                    <li v-if="this.resourceLevel == 'instance'">
                        <a class="dropdown-item" target="blank"
                            v-bind:href="getApiUrl('/tags?simplify')">/tags?simplify</a>
                    </li>
                    <li>
                        <a class="dropdown-item" target="blank"
                            v-bind:href="getApiUrl('/metadata?expand')">/metadata</a>
                    </li>
                    <li>
                        <a class="dropdown-item" target="blank"
                            v-bind:href="getApiUrl('/attachments?expand')">/attachments</a>
                    </li>
                </ul>
            </div>
        </div>
        <div class="btn-group">
            <div class="dropdown">
                <button v-if="hasSendTo" class="dropdown btn btn-sm btn-secondary m-1 dropdown-toggle" type="button"
                    id="sendToDropdownMenuId" data-bs-toggle="dropdown" aria-expanded="false">
                    <span data-bs-toggle="tooltip" title="Send">
                        <i class="bi bi-send"></i>
                    </span>
                </button>
                <ul class="dropdown-menu bg-dropdown" aria-labelledby="sendToDropdownMenuId" v-if="hasSendTo">
                    <li v-if="hasSendToPeers" class="dropdown-submenu">
                        <a class="dropdown-item" @click="toggleSubMenu" href="#">
                            Orthanc peers
                            <i class="bi bi-caret-down"></i>
                        </a>
                        <ul class="dropdown-menu bg-dropdown">
                            <li v-for="peer in orthancPeers" :key="peer">
                                <a class="dropdown-item" @click="sendToOrthancPeer(peer)">{{ peer }}</a>
                            </li>
                        </ul>
                    </li>
                    <li v-if="hasSendToPeersWithTransfer" class="dropdown-submenu">
                        <a class="dropdown-item" @click="toggleSubMenu" href="#">
                            Advanced transfers
                            <i class="bi bi-caret-down"></i>
                        </a>
                        <ul class="dropdown-menu bg-dropdown">
                            <li v-for="peer in orthancPeers" :key="peer">
                                <a class="dropdown-item" @click="sendToOrthancPeerWithTransfers(peer)">{{ peer }}</a>
                            </li>
                        </ul>
                    </li>
                    <li v-if="hasSendToDicomWeb" class="dropdown-submenu">
                        <a class="dropdown-item" @click="toggleSubMenu" href="#">
                            Dicom WEB
                            <i class="bi bi-caret-down"></i>
                        </a>
                        <ul class="dropdown-menu bg-dropdown">
                            <li v-for="dwServer in targetDicomWebServers" :key="dwServer">
                                <a class="dropdown-item" @click="sendToDicomWebServer(dwServer)">{{ dwServer }}</a>
                            </li>
                        </ul>
                    </li>
                    <li v-if="hasSendToDicomModalities" class="dropdown-submenu">
                        <a class="dropdown-item" @click="toggleSubMenu" href="#">
                            DICOM
                            <i class="bi bi-caret-down"></i>
                        </a>
                        <ul class="dropdown-menu bg-dropdown">
                            <li v-for="modality in targetDicomModalities" :key="modality">
                                <a class="dropdown-item" @click="sendToDicomModality(modality)">{{ modality }}</a>
                            </li>
                        </ul>
                    </li>
                </ul>
            </div>
        </div>
    </div>
</template>

<style>
.bg-dropdown {
    background-color: rgb(220, 220, 220);
}

.dropdown-submenu {
    position: relative;
}

.dropdown-submenu .dropdown-menu {
    top: 0;
    right: 100%;
    margin-top: -1px;
}
</style>