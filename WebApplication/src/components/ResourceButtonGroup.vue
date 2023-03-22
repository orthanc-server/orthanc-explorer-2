<script>
import Modal from "./Modal.vue"
import ShareModal from "./ShareModal.vue"
import ModifyModal from "./ModifyModal.vue"
import $ from "jquery"
import { mapState } from "vuex"
import api from "../orthancApi"
import resourceHelpers from "../helpers/resource-helpers"
import clipboardHelpers from "../helpers/clipboard-helpers"
import TokenLinkButton from "./TokenLinkButton.vue"


export default {
    props: ["resourceOrthancId", "resourceDicomUid", "resourceLevel", "customClass", "seriesMainDicomTags", "studyMainDicomTags", "patientMainDicomTags", "instanceTags"],
    setup() {
        return {
        }
    },

    data() {
        return {
            isDeleteModalVisible: false
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
            if (this.resourceLevel == 'bulk') {
                api.deleteResources(this.resourcesOrthancId);
                window.location.reload();
            } else {
                api.deleteResource(this.resourceLevel, this.resourceOrthancId)
                    .then(() => {
                        this.$emit("deletedResource");
                    })
                    .catch((reason) => {
                        console.error("failed to delete resource : ", this.resourceOrthancId, reason);
                    });
            }
        },
        getApiUrl(subRoute) {
            return api.getApiUrl(this.resourceLevel, this.resourceOrthancId, subRoute);
        },
        async sendToDicomWebServer(server) {
            const jobId = await api.sendToDicomWebServer(this.resourcesOrthancId, server);
            this.$store.dispatch('jobs/addJob', { jobId: jobId, name: 'Send to DicomWeb (' + server + ')' });
        },
        async sendToOrthancPeer(peer) {
            const jobId = await api.sendToOrthancPeer(this.resourcesOrthancId, peer);
            this.$store.dispatch('jobs/addJob', { jobId: jobId, name: 'Send to Peer (' + peer + ')' });
        },
        async sendToOrthancPeerWithTransfers(peer) {
            const jobId = await api.sendToOrthancPeerWithTransfers(this.resourcesForTransfer, peer);
            this.$store.dispatch('jobs/addJob', { jobId: jobId, name: 'Transfer to Peer (' + peer + ')' });
        },
        async sendToDicomModality(modality) {
            const jobId = await api.sendToDicomModality(this.resourcesOrthancId, modality);
            this.$store.dispatch('jobs/addJob', { jobId: jobId, name: 'Send to DICOM (' + modality + ')' });
        },
        capitalizeFirstLetter(level) {
            return level.charAt(0).toUpperCase() + level.slice(1);
        },
        copyIdToClipboard() {
            clipboardHelpers.copyToClipboard(this.resourceOrthancId);
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
    watch: {
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
            installedPlugins: state => state.configuration.installedPlugins,
            targetDicomWebServers: state => state.configuration.targetDicomWebServers,
            targetDicomModalities: state => state.configuration.targetDicomModalities,
            selectedStudiesIds: state => state.studies.selectedStudiesIds,
            selectedStudies: state => state.studies.selectedStudies,
            orthancPeers: state => state.configuration.orthancPeers
        }),
        hasSendTo() {
            return this.uiOptions.EnableSendTo &&
                (this.hasSendToDicomWeb || this.hasSendToPeers || this.hasSendToDicomModalities || this.hasSendToPeersWithTransfer);
        },
        isSendToEnabled() {
            if (this.resourceLevel == 'bulk') {
                return this.selectedStudiesIds.length > 0
            } else {
                return true;
            }
        },
        isDeleteEnabled() {
            if (this.resourceLevel == 'bulk') {
                return this.selectedStudiesIds.length > 0
            } else {
                return true;
            }
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
            if (this.resourceLevel == 'bulk') {
                const selectedStudiesDicomIds = this.selectedStudies.map(s => s['MainDicomTags']['StudyInstanceUID']);
                const url = api.getStoneViewerUrlForBulkStudies(selectedStudiesDicomIds);
                return url;
            } else {
                return api.getStoneViewerUrl(this.resourceLevel, this.resourceDicomUid);
            }
        },
        hasStoneViewerButton() {
            return this.hasStoneViewer && (this.resourceLevel == 'study' || this.resourceLevel == 'bulk');
        },
        isStoneViewerButtonEnabled() {
            return (this.resourceLevel == 'study' || (this.resourceLevel == 'bulk' && this.selectedStudiesIds.length > 0));
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
                "instance": "delete_instance_title",
                "bulk": "delete_studies_title"
            }
            return texts[this.resourceLevel];
        },
        deleteResourceBody() {
            const texts = {
                "study": "delete_study_body",
                "series": "delete_series_body",
                "instance": "delete_instance_body",
                "bulk": "delete_studies_body"
            }
            return texts[this.resourceLevel];
        },
        isAnonymizationEnabled() {
            if (this.resourceLevel == "study" || this.resourceLevel == "series") {
                return this.uiOptions.EnableAnonymization;
            } else {
                return false;
            }
        },
        isModificationEnabled() {
            if (this.resourceLevel == "study" || this.resourceLevel == "series") {
                return this.uiOptions.EnableModification;
            } else {
                return false;
            }
        },
        resourcesOrthancId() {
            if (this.resourceLevel == 'bulk') {
                return this.selectedStudiesIds;
            } else {
                return [this.resourcesOrthancId];
            }
        },
        resourcesForTransfer() {
            if (this.resourceLevel == 'bulk') {
                return this.selectedStudiesIds.map(id => ({ "Level": "Study", "ID": id }));
            } else {
                return [{ "Level": this.capitalizeFirstLetter(this.resourceLevel), "ID": this.resourceOrthancId }];
            }
        },
        computedResourceLevel() {
            if (this.resourceLevel == 'bulk') {
                
                return "study";
            } else {
                return this.resourceLevel;
            }
        }
    },
    components: { Modal, ShareModal, ModifyModal, TokenLinkButton }
}
</script>

<template>
    <div>
        <div class="btn-group">
            <span v-for="viewer in uiOptions.ViewersOrdering" :key="viewer">
                <TokenLinkButton v-if="hasMedDreamViewer && viewer == 'meddream' && this.resourceLevel == 'study'"
                    :iconClass="medDreamViewerIcon" :level="this.resourceLevel" :linkUrl="medDreamViewerUrl"
                    :resourcesOrthancId="resourcesOrthancId" :title="$t('view_in_meddream')"
                    :tokenType="'meddream-instant-link'" :opensInNewTab="true">
                </TokenLinkButton>

                <TokenLinkButton
                    v-if="hasOsimisViewer && viewer == 'osimis-web-viewer' && (this.resourceLevel == 'study' || this.resourceLevel == 'series')"
                    :iconClass="osimisViewerIcon" :level="this.resourceLevel" :linkUrl="osimisViewerUrl"
                    :resourcesOrthancId="resourcesOrthancId" :title="$t('view_in_osimis')"
                    :tokenType="'viewer-instant-link'" :opensInNewTab="true">
                </TokenLinkButton>

                <TokenLinkButton v-if="viewer == 'stone-webviewer' && hasStoneViewerButton"
                    :disabled="!isStoneViewerButtonEnabled"
                    :iconClass="stoneViewerIcon" :level="computedResourceLevel" :linkUrl="stoneViewerUrl"
                    :resourcesOrthancId="resourcesOrthancId" :title="$t('view_in_stone')"
                    :tokenType="'viewer-instant-link'" :opensInNewTab="true">
                </TokenLinkButton>

                <a v-if="hasOhifViewer && viewer == 'ohif' && this.resourceLevel == 'study'"
                    class="btn btn-sm btn-secondary m-1" type="button" data-bs-toggle="tooltip" :title="$t('view_in_ohif')"
                    target="blank" v-bind:href="ohifViewerUrl">
                    <i :class="ohifViewerIcon"></i>
                </a>
            </span>
            <a v-if="this.resourceLevel == 'instance'" class="btn btn-sm btn-secondary m-1" type="button"
                data-bs-toggle="tooltip" :title="`${$t('preview')}`" target="blank" v-bind:href="instancePreviewUrl">
                <i class="bi bi-binoculars"></i>
            </a>
        </div>
        <div class="btn-group" v-if="this.resourceLevel != 'bulk'">
            <TokenLinkButton v-if="uiOptions.EnableDownloadZip && this.resourceLevel != 'instance'"
                :iconClass="'bi bi-download'" :level="this.resourceLevel" :linkUrl="downloadZipUrl"
                :resourcesOrthancId="[resourceOrthancId]" :title="$t('download_zip')" :tokenType="'download-instant-link'">
            </TokenLinkButton>
            <TokenLinkButton v-if="uiOptions.EnableDownloadDicomDir && this.resourceLevel != 'instance'"
                :iconClass="'bi bi-box-arrow-down'" :level="this.resourceLevel" :linkUrl="downloadDicomDirUrl"
                :resourcesOrthancId="[resourceOrthancId]" :title="$t('download_dicomdir')"
                :tokenType="'download-instant-link'">
            </TokenLinkButton>
            <TokenLinkButton v-if="uiOptions.EnableDownloadDicomFile && this.resourceLevel == 'instance'"
                :iconClass="'bi bi-download'" :level="this.resourceLevel" :linkUrl="instanceDownloadUrl"
                :resourcesOrthancId="[resourceOrthancId]" :title="$t('download_dicom_file')"
                :tokenType="'download-instant-link'">
            </TokenLinkButton>
        </div>
        <div class="btn-group" >
            <button v-if="uiOptions.EnableDeleteResources" class="btn btn-sm btn-secondary m-1" type="button"
                data-bs-toggle="modal" v-bind:data-bs-target="'#delete-modal-' + this.resourceOrthancId" :disabled="!isDeleteEnabled">
                <i class="bi bi-trash" data-bs-toggle="tooltip" :title="$t('delete')" ></i>
            </button>
            <Modal v-if="uiOptions.EnableDeleteResources" :id="'delete-modal-' + this.resourceOrthancId"
                :headerText="$t(this.deleteResourceTitle) + ' ' + this.resourceTitle" :okText="$t('delete')"
                :cancelText="$t('cancel')" :bodyText="$t(this.deleteResourceBody)" @ok="deleteResource($event)">
            </Modal>
        </div>
        <div class="btn-group" v-if="this.resourceLevel != 'bulk'">
            <button v-if="uiOptions.EnableShares" class="btn btn-sm btn-secondary m-1" type="button" data-bs-toggle="modal"
                v-bind:data-bs-target="'#share-modal-' + this.resourceOrthancId">
                <i class="bi bi-share" data-bs-toggle="tooltip" :title="$t('share.button_title')"></i>
            </button>
            <ShareModal v-if="uiOptions.EnableShares" :id="'share-modal-' + this.resourceOrthancId"
                :orthancId="this.resourceOrthancId" :studyMainDicomTags="this.studyMainDicomTags"
                :patientMainDicomTags="this.patientMainDicomTags"></ShareModal>
        </div>
        <div class="btn-group" v-if="this.resourceLevel != 'bulk'">
            <button v-if="isModificationEnabled" class="btn btn-sm btn-secondary m-1" type="button" data-bs-toggle="modal"
                v-bind:data-bs-target="'#modify-modal-' + this.resourceOrthancId">
                <i class="bi bi-pencil" data-bs-toggle="tooltip" :title="$t('modify.modify_button_title')"></i>
            </button>
            <ModifyModal v-if="isModificationEnabled" :id="'modify-modal-' + this.resourceOrthancId"
                :orthancId="this.resourceOrthancId" :resourceLevel="this.resourceLevel"
                :seriesMainDicomTags="this.seriesMainDicomTags" :studyMainDicomTags="this.studyMainDicomTags"
                :patientMainDicomTags="this.patientMainDicomTags" :isAnonymization="false"></ModifyModal>
        </div>
        <div class="btn-group" v-if="this.resourceLevel != 'bulk'">
            <button v-if="isAnonymizationEnabled" class="btn btn-sm btn-secondary m-1" type="button" data-bs-toggle="modal"
                v-bind:data-bs-target="'#anonymize-modal-' + this.resourceOrthancId">
                <i class="bi bi-person-slash" data-bs-toggle="tooltip" :title="$t('modify.anonymize_button_title')"></i>
            </button>
            <ModifyModal v-if="isAnonymizationEnabled" :id="'anonymize-modal-' + this.resourceOrthancId"
                :orthancId="this.resourceOrthancId" :resourceLevel="this.resourceLevel"
                :seriesMainDicomTags="this.seriesMainDicomTags" :studyMainDicomTags="this.studyMainDicomTags"
                :patientMainDicomTags="this.patientMainDicomTags" :isAnonymization="true"></ModifyModal>
        </div>
        <div class="btn-group" v-if="this.resourceLevel != 'bulk'">
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
                        <button class="dropdown-item" href="#" @click="copyIdToClipboard">{{ $t('copy_orthanc_id')
                        }}</button>
                    </li>
                    <li>
                        <TokenLinkButton :linkType="'dropdown-item'" :level="this.resourceLevel" :linkUrl="getApiUrl('')"
                            :resourcesOrthancId="[resourceOrthancId]" :title="'/'" :tokenType="'download-instant-link'"
                            :opensInNewTab="true">
                        </TokenLinkButton>
                    </li>
                    <li v-if="this.resourceLevel == 'instance'">
                        <TokenLinkButton :linkType="'dropdown-item'" :level="this.resourceLevel" :linkUrl="getApiUrl('/tags?simplify')"
                            :resourcesOrthancId="[resourceOrthancId]" :title="'/tags?simplify'" :tokenType="'download-instant-link'"
                            :opensInNewTab="true">
                        </TokenLinkButton>
                    </li>
                    <li>
                        <TokenLinkButton :linkType="'dropdown-item'" :level="this.resourceLevel" :linkUrl="getApiUrl('/metadata?expand')"
                            :resourcesOrthancId="[resourceOrthancId]" :title="'/metadata?expand'" :tokenType="'download-instant-link'"
                            :opensInNewTab="true">
                        </TokenLinkButton>
                    </li>
                    <li>
                        <TokenLinkButton :linkType="'dropdown-item'" :level="this.resourceLevel" :linkUrl="getApiUrl('/statistics')"
                            :resourcesOrthancId="[resourceOrthancId]" :title="'/statistics'" :tokenType="'download-instant-link'"
                            :opensInNewTab="true">
                        </TokenLinkButton>
                    </li>
                    <li>
                        <TokenLinkButton :linkType="'dropdown-item'" :level="this.resourceLevel" :linkUrl="getApiUrl('/attachments?expand')"
                            :resourcesOrthancId="[resourceOrthancId]" :title="'/attachments?expand'" :tokenType="'download-instant-link'"
                            :opensInNewTab="true">
                        </TokenLinkButton>
                    </li>
                </ul>
            </div>
        </div>
        <div class="btn-group">
            <div class="dropdown">
                <button v-if="hasSendTo" class="dropdown btn btn-sm btn-secondary m-1 dropdown-toggle" type="button"
                    id="sendToDropdownMenuId" data-bs-toggle="dropdown" aria-expanded="false" :disabled="!isSendToEnabled">
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