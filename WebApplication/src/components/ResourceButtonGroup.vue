<script>
import Modal from "./Modal.vue"
import ShareModal from "./ShareModal.vue"
import ModifyModal from "./ModifyModal.vue"
import $ from "jquery"
import { mapState } from "vuex"
import api from "../orthancApi"
import resourceHelpers from "../helpers/resource-helpers"
import clipboardHelpers from "../helpers/clipboard-helpers"


export default {
    props: ["resourceOrthancId", "resourceDicomUid", "resourceLevel", "customClass", "seriesMainDicomTags", "studyMainDicomTags", "patientMainDicomTags", "instanceTags"],
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
        },
        copyIdToClipboard() {
            clipboardHelpers.copyToClipboard(this.resourceOrthancId);
        },
        async openMedDream(event) {
            if (this.uiOptions.EnableMedDreamInstantLinks) {
                event.preventDefault();

                console.log("opening MedDream, requesting an instant link token");
                let link = await api.getMedDreamInstantLink(this.resourceDicomUid);
                window.open(link, 'blank');
            }
        },
        getViewerIcon(forViewer) {
            const orderedViewerList = this.uiOptions.ViewersOrdering;
            const viewersIcons = this.uiOptions.ViewersIcons;

            for (let viewer of orderedViewerList) {
                if (viewer == forViewer) {
                    if (this.hasOsimisViewer && forViewer == "osimis-web-viewer") {
                        return viewersIcons[viewer];
                    }

                    if (this.hasStoneViewer && forViewer == "stone-webviewer") {
                        return viewersIcons[viewer];
                    }

                    if (this.hasOhifViewer && forViewer == "ohif") {
                        return viewersIcons[viewer];
                    }

                    if (this.hasMedDreamViewer && forViewer == "meddream") {
                        return viewersIcons[viewer];
                    }
                }
            }
            return "bi bi-eye";
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
            if (this.uiOptions.EnableMedDreamInstantLinks) {
                return "#";
            } else {
                return this.uiOptions.MedDreamViewerPublicRoot + "?study=" + this.resourceDicomUid;
            }
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
        resourceTitle() {
            return resourceHelpers.getResourceTitle(this.resourceLevel, this.patientMainDicomTags, this.studyMainDicomTags, this.seriesMainDicomTags, this.instanceTags);
        },
        osimisViewerIcon() {
            return this.getViewerIcon("osimis-web-viewer");
        },
        stoneViewerIcon() {
            return this.getViewerIcon("stone-webviewer");

        },
        medDreamViewerIcon() {
            return this.getViewerIcon("meddream");

        },
        ohifViewerIcon() {
            return this.getViewerIcon("ohif");
        },
        deleteResourceTitle() {
            const texts = {
                "study": "delete_study_title",
                "series": "delete_series_title",
                "instance": "delete_instance_title"
            }
            return texts[this.resourceLevel];
        },
        deleteResourceBody() {
            const texts = {
                "study": "delete_study_body",
                "series": "delete_series_body",
                "instance": "delete_instance_body"
            }
            return texts[this.resourceLevel];
        },
        isAnonymizationEnabled() {
            if (this.resourceLevel == "study") {
                return this.uiOptions.EnableStudyAnonimization;
            } else if (this.resourceLevel == "series") {
                return this.uiOptions.EnableSeriesAnonimization;
            } else {
                return false;
            }
        },
        isModificationEnabled() {
            if (this.resourceLevel == "study") {
                return this.uiOptions.EnableStudyModification;
            } else if (this.resourceLevel == "series") {
                return this.uiOptions.EnableSeriesModification;
            } else {
                return false;
            }
        }
    },
    components: { Modal, ShareModal, ModifyModal }
}
</script>

<template>
    <div>
        <div class="btn-group">
            <span v-for="viewer in uiOptions.ViewersOrdering" :key="viewer">
                <a v-if="hasMedDreamViewer && viewer == 'meddream' && this.resourceLevel == 'study'"
                    class="btn btn-sm btn-secondary m-1" type="button" data-bs-toggle="tooltip"
                    :title="$t('view_in_meddream')" target="blank" @click="openMedDream"
                    v-bind:href="medDreamViewerUrl">
                    <i :class="medDreamViewerIcon"></i>
                </a>
                <a v-if="hasOsimisViewer && viewer == 'osimis-web-viewer' && (this.resourceLevel == 'study' || this.resourceLevel == 'series')"
                    class="btn btn-sm btn-secondary m-1" type="button" data-bs-toggle="tooltip"
                    :title="$t('view_in_osimis')" target="blank" v-bind:href="osimisViewerUrl">
                    <i :class="osimisViewerIcon"></i>
                </a>
                <a v-if="hasStoneViewer && viewer == 'stone-webviewer' && this.resourceLevel == 'study'"
                    class="btn btn-sm btn-secondary m-1" type="button" data-bs-toggle="tooltip"
                    :title="$t('view_in_stone')" target="blank" v-bind:href="stoneViewerUrl">
                    <i :class="stoneViewerIcon"></i>
                </a>
                <a v-if="hasOhifViewer && viewer == 'ohif' && this.resourceLevel == 'study'"
                    class="btn btn-sm btn-secondary m-1" type="button" data-bs-toggle="tooltip"
                    :title="$t('view_in_ohif')" target="blank" v-bind:href="ohifViewerUrl">
                    <i :class="ohifViewerIcon"></i>
                </a>
            </span>
            <a v-if="this.resourceLevel == 'instance'" class="btn btn-sm btn-secondary m-1" type="button"
                data-bs-toggle="tooltip" :title="`${$t('preview')}`" target="blank" v-bind:href="instancePreviewUrl">
                <i class="bi bi-binoculars"></i>
            </a>
        </div>
        <div class="btn-group">
            <a v-if="uiOptions.EnableDownloadZip && this.resourceLevel != 'instance'"
                class="btn btn-sm btn-secondary m-1" type="button" data-bs-toggle="tooltip"
                :title="`${$t('download_zip')}`" v-bind:href="downloadZipUrl">
                <i class="bi bi-download"></i>
            </a>
            <a v-if="uiOptions.EnableDownloadDicomDir && this.resourceLevel != 'instance'"
                class="btn btn-sm btn-secondary m-1" type="button" data-bs-toggle="tooltip"
                :title="`${$t('download_dicomdir')}`" v-bind:href="downloadDicomDirUrl">
                <i class="bi bi-box-arrow-down"></i>
            </a>
            <a v-if="uiOptions.EnableDownloadDicomFile && this.resourceLevel == 'instance'"
                class="btn btn-sm btn-secondary m-1" type="button" data-bs-toggle="tooltip"
                :title="`${$t('download_dicom_file')}`" v-bind:href="instanceDownloadUrl">
                <i class="bi bi-download"></i>
            </a>
        </div>
        <div class="btn-group">
            <!-- <button
            v-if="uiOptions.EnableAnonymize"
            class="btn btn-sm btn-secondary m-1"
            type="button"
            data-bs-toggle="tooltip"
            :title="`${$t('anonymize')}`"
        >
            <i class="bi bi-person-x"></i>
        </button> -->
            <button v-if="uiOptions.EnableDeleteResources" class="btn btn-sm btn-secondary m-1" type="button"
                data-bs-toggle="modal" v-bind:data-bs-target="'#delete-modal-' + this.resourceOrthancId">
                <i class="bi bi-trash" data-bs-toggle="tooltip" :title="$t('delete')"></i>
            </button>
            <Modal v-if="uiOptions.EnableDeleteResources" :id="'delete-modal-' + this.resourceOrthancId"
                :headerText="$t(this.deleteResourceTitle) + ' ' + this.resourceTitle" :okText="$t('delete')"
                :cancelText="$t('cancel')" :bodyText="$t(this.deleteResourceBody)" @ok="deleteResource($event)">
            </Modal>
        </div>
        <div class="btn-group">
            <button v-if="uiOptions.EnableShares" class="btn btn-sm btn-secondary m-1" type="button"
                data-bs-toggle="modal" v-bind:data-bs-target="'#share-modal-' + this.resourceOrthancId">
                <i class="bi bi-share" data-bs-toggle="tooltip" :title="$t('share.button_title')"></i>
            </button>
            <ShareModal v-if="uiOptions.EnableShares" :id="'share-modal-' + this.resourceOrthancId"
                :orthancId="this.resourceOrthancId" :studyMainDicomTags="this.studyMainDicomTags"
                :patientMainDicomTags="this.patientMainDicomTags"></ShareModal>
        </div>
        <div class="btn-group">
            <button v-if="isModificationEnabled" class="btn btn-sm btn-secondary m-1" type="button"
                data-bs-toggle="modal" v-bind:data-bs-target="'#modify-modal-' + this.resourceOrthancId">
                <i class="bi bi-pencil" data-bs-toggle="tooltip" :title="$t('modify.modify_button_title')"></i>
            </button>
            <ModifyModal v-if="isModificationEnabled" :id="'modify-modal-' + this.resourceOrthancId"
                :orthancId="this.resourceOrthancId" :resourceLevel="this.resourceLevel"
                :seriesMainDicomTags="this.seriesMainDicomTags" :studyMainDicomTags="this.studyMainDicomTags"
                :patientMainDicomTags="this.patientMainDicomTags" :isAnonymization="false"></ModifyModal>
        </div>
        <div class="btn-group">
            <button v-if="isAnonymizationEnabled" class="btn btn-sm btn-secondary m-1" type="button"
                data-bs-toggle="modal" v-bind:data-bs-target="'#anonymize-modal-' + this.resourceOrthancId">
                <i class="bi bi-person-slash" data-bs-toggle="tooltip" :title="$t('modify.anonymize_button_title')"></i>
            </button>
            <ModifyModal v-if="isAnonymizationEnabled" :id="'anonymize-modal-' + this.resourceOrthancId"
                :orthancId="this.resourceOrthancId" :resourceLevel="this.resourceLevel"
                :seriesMainDicomTags="this.seriesMainDicomTags" :studyMainDicomTags="this.studyMainDicomTags"
                :patientMainDicomTags="this.patientMainDicomTags" :isAnonymization="true"></ModifyModal>
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
                        <button class="dropdown-item" href="#"
                            @click="copyIdToClipboard">{{ $t('copy_orthanc_id') }}</button>
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
                    <span data-bs-toggle="tooltip" :title="$t('send_to.button_title')">
                        <i class="bi bi-send"></i>
                    </span>
                </button>
                <ul class="dropdown-menu bg-dropdown" aria-labelledby="sendToDropdownMenuId" v-if="hasSendTo">
                    <li v-if="hasSendToPeers" class="dropdown-submenu">
                        <a class="dropdown-item" @click="toggleSubMenu" href="#">
                            {{ $t('send_to.orthanc_peer') }}
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
                            {{ $t('send_to.transfers') }}
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
                            {{ $t('send_to.dicom_web') }}
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
                            {{ $t('send_to.dicom') }}
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