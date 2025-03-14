<script>
import Modal from "./Modal.vue"
import ShareModal from "./ShareModal.vue"
import ModifyModal from "./ModifyModal.vue"
import AddSeriesModal from "./AddSeriesModal.vue"
import $ from "jquery"
import { mapState } from "vuex"
import api from "../orthancApi"
import resourceHelpers from "../helpers/resource-helpers"
import clipboardHelpers from "../helpers/clipboard-helpers"
import TokenLinkButton from "./TokenLinkButton.vue"
import BulkLabelsModal from "./BulkLabelsModal.vue"
import SourceType from '../helpers/source-type';
import axios from "axios"


export default {
    props: ["resourceOrthancId", "resourceDicomUid", "resourceLevel", "customClass", "seriesMainDicomTags", "studyMainDicomTags", "patientMainDicomTags", "instanceTags", "instanceHeaders", "studySeries", "seriesInstances"],
    setup() {
        return {
        }
    },

    data() {
        return {
            isBulkLabelModalVisible: false,
            isWsiSeries: false,
            modalitiesList: []
        };
    },
    watch: {
        async studySeries(newValue, oldValue) {
            if (this.resourceLevel == 'study') {
                // build the modalitiesList to enable/disable viewers
                let modalitiesSet = new Set();
                for (let seriesDetail of this.studySeries) {
                    modalitiesSet.add(seriesDetail["MainDicomTags"]["Modality"])
                }
                this.modalitiesList = Array.from(modalitiesSet);
            } else if (this.resourceLevel == 'bulk') {
                // build the modalitiesList to enable/disable viewers
                let modalitiesSet = new Set();
                for (let selectedStudyId of this.selectedStudiesIds) {
                    let seriesDetails = await api.getStudySeries(selectedStudyId);
                    for (let seriesDetail of seriesDetails) {
                        modalitiesSet.add(seriesDetail["MainDicomTags"]["Modality"])
                    }
                }
                this.modalitiesList = Array.from(modalitiesSet);
            }
        },
        async seriesInstances(newValue, oldValue) {
            if (this.studiesSourceType == SourceType.LOCAL_ORTHANC && this.resourceLevel == 'series') {
                let firstInstanceTags = await api.getSimplifiedInstanceTags(this.seriesInstances[0]['ID']);
                this.modalitiesList = [firstInstanceTags["Modality"]];
                this.isWsiSeries = firstInstanceTags["SOPClassUID"] == "1.2.840.10008.5.1.4.1.1.77.1.6" || firstInstanceTags["Modality"] == "SM";
            }
        },
    },

    async mounted() {
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
                api.deleteResources(this.resourcesOrthancId)
                    .then(() => {
                        window.location.reload();
                    })
                    .catch((reason) => {
                        console.error("failed to delete resources : ", this.resourceOrthancId, reason);
                    });
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
        async retrieve() {
            if (this.studiesSourceType == SourceType.REMOTE_DICOM) {
                
                if (this.resourceLevel == "bulk") {
                    let moveQueries = this.selectedStudies.map(s => { return {"StudyInstanceUID": s['ID'], "PatientID": s['PatientMainDicomTags']['PatientID']}});

                    const jobId = await api.remoteDicomRetrieveResources("Study", this.studiesRemoteSource, moveQueries);
                    this.$store.dispatch('jobs/addJob', { jobId: jobId, name: 'Retrieve ' + this.selectedStudies.length  + ' studies from (' + this.studiesRemoteSource + ')'});
                } else {
                    let moveQuery = {
                        "StudyInstanceUID": this.studyMainDicomTags.StudyInstanceUID,
                        "PatientID": this.patientMainDicomTags.PatientID
                    };

                    if (this.resourceLevel == 'series' || this.resourceLevel == 'instance') {
                        moveQuery["SeriesInstanceUID"] = this.seriesMainDicomTags.SeriesInstanceUID;
                    }
                    if (this.resourceLevel == 'instance') {
                        moveQuery["SOPInstanceUID"] = this.instanceTags["0008,0018"].Value;
                    }

                    const jobId = await api.remoteDicomRetrieveResource(this.capitalizeFirstLetter(this.resourceLevel), this.studiesRemoteSource, moveQuery);
                    this.$store.dispatch('jobs/addJob', { jobId: jobId, name: 'Retrieve ' + this.capitalizeFirstLetter(this.resourceLevel) + ' from (' + this.studiesRemoteSource + ')'});
                }
            } else if (this.studiesSourceType == SourceType.REMOTE_DICOM_WEB) {
                let resources;
                if (this.resourceLevel == "bulk") {
                    resources = this.selectedStudies.map(s => { return {"Study": s['ID']}});
                } else if (this.resourceLevel == "study") {
                    resources = [{"Study": this.studyMainDicomTags.StudyInstanceUID}];
                } else if (this.resourceLevel == "series") {
                    resources = [{
                        "Study": this.studyMainDicomTags.StudyInstanceUID,
                        "Series": this.seriesMainDicomTags.SeriesInstanceUID
                    }];
                } else if (this.resourceLevel == "instance") {
                    resources = [{
                        "Study": this.studyMainDicomTags.StudyInstanceUID,
                        "Series": this.seriesMainDicomTags.SeriesInstanceUID,
                        "Instance": this.instanceTags.SOPInstanceUID
                    }];
                }
                const jobId = await api.wadoRsRetrieve(this.studiesRemoteSource, resources);
                this.$store.dispatch('jobs/addJob', { jobId: jobId, name: 'Retrieve ' + this.capitalizeFirstLetter(this.resourceLevel) + ' from (' + this.studiesRemoteSource + ')'});
            }
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

                    if (this.hasVolView && forViewer == "volview") {
                        return viewersIcons[viewer];
                    }

                    if (this.hasOhifViewer && ["ohif", "ohif-vr", "ohif-tmtv", "ohif-seg", "ohif-micro"].includes(forViewer)) {
                        return viewersIcons[viewer];
                    }

                    if (this.hasMedDreamViewer && forViewer == "meddream") {
                        return viewersIcons[viewer];
                    }

                    if (this.hasWsiViewer && forViewer == "wsi") {
                        return viewersIcons[viewer];
                    }
                }
            }
            return "bi bi-eye";
        },
        showBulkLabelModal() {
            // this modal is re-created everytime we open it (to interface correctly with bootstrap5-tags
            this.isBulkLabelModalVisible = true;
            // wait that the DOM element is created !
            setTimeout(() => {
                this.$refs['bulk-label-modal'].showModal();
            }, 50);
        },
        async onBulkModalClosed() {
            this.isBulkLabelModalVisible = false;
            await this.$store.dispatch('labels/refresh');
        },
        getOhifViewerUrl(mode) {
            if (this.resourceLevel == 'bulk') {
                const selectedStudiesDicomIds = this.selectedStudies.map(s => s['MainDicomTags']['StudyInstanceUID']);
                const url = api.getOhifViewerUrlForDicomWebBulkStudies(mode, selectedStudiesDicomIds);
                return url;
            } else {
                if (this.ohifDataSource == 'dicom-web') {
                    return api.getOhifViewerUrlForDicomWeb(mode, this.resourceDicomUid);
                } else if (this.ohifDataSource == 'dicom-json') {
                    return api.getOhifViewerUrlForDicomJson(mode, this.resourceOrthancId);
                }
            }
        },
        async customButtonClicked(customButton) {
            let url = resourceHelpers.replaceResourceTagsInString(customButton.Url, this.patientMainDicomTags, this.studyMainDicomTags, this.seriesMainDicomTags, this.instanceTags, this.resourceOrthancId);
            let response;

            if (customButton.HttpMethod == 'PUT' || customButton.HttpMethod == 'POST') {
                let payload = resourceHelpers.replaceResourceTagsInJson(customButton.Payload, this.patientMainDicomTags, this.studyMainDicomTags, this.seriesMainDicomTags, this.instanceTags, this.resourceOrthancId);

                if (customButton.HttpMethod == 'PUT') {
                    response = axios.put(url, payload);
                } else if (customButton.HttpMethod == 'POST') {
                    response = axios.post(url, payload);
                }
            } else if (customButton.HttpMethod == 'DELETE') {
                response = axios.delete(url);
            }

            await response;
            if (customButton.Refresh) {
                await this.$store.dispatch('labels/refresh');
                if (this.resourceLevel == 'study') {
                    const study = await api.getStudy(this.resourceOrthancId); 
                    await this.$store.dispatch('studies/reloadStudy', {
                        'studyId': this.resourceOrthancId,
                        'study': study
                    });
                    this.messageBus.emit("study-updated-" + this.resourceOrthancId, study);
                } else {
                    console.warn("Refresh " + this.resourceLevel + " currently not supported");
                }
            }
        }
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
            studiesSourceType: state => state.studies.sourceType,
            studiesRemoteSource: state => state.studies.remoteSource,
            ohifDataSource: state => state.configuration.ohifDataSource,
            installedPlugins: state => state.configuration.installedPlugins,
            targetDicomWebServers: state => state.configuration.targetDicomWebServers,
            targetDicomModalities: state => state.configuration.targetDicomModalities,
            selectedStudiesIds: state => state.studies.selectedStudiesIds,
            selectedStudies: state => state.studies.selectedStudies,
            orthancPeers: state => state.configuration.orthancPeers,
            tokens: state => state.configuration.tokens,
            system: state => state.configuration.system,
        }),
        componentId() {
            if (this.resourceLevel == 'bulk') {
                return 'bulk-id';
            } else {
                return this.resourceOrthancId;
            }
        },
        hasSendTo() {
            return this.studiesSourceType == SourceType.LOCAL_ORTHANC && this.uiOptions.EnableSendTo &&
                (this.hasSendToDicomWeb || this.hasSendToPeers || this.hasSendToDicomModalities || this.hasSendToPeersWithTransfer);
        },
        isSendToEnabled() {
            if (this.resourceLevel == 'bulk') {
                return this.selectedStudiesIds.length > 0
            } else {
                return true;
            }
        },
        hasLabelsButton() {
            return this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
                this.uiOptions.EnableEditLabels && this.resourceLevel == 'bulk';
        },
        isLabelsEnabled() {
            if (this.resourceLevel == 'bulk') {
                return this.selectedStudiesIds.length > 0
            } else {
                return false;
            }
        },
        hasDeleteButton() {
            return this.studiesSourceType == SourceType.LOCAL_ORTHANC && 
                this.uiOptions.EnableDeleteResources;
        },
        isDeleteEnabled() {
            if (this.resourceLevel == 'bulk') {
                return this.selectedStudiesIds.length > 0
            } else {
                return true;
            }
        },
        hasShareButton() {
            return this.studiesSourceType == SourceType.LOCAL_ORTHANC && 
                (this.uiOptions.EnableShares && "authorization" in this.installedPlugins && ['bulk', 'study'].includes(this.resourceLevel));
        },
        isShareEnabled() {
            if (this.resourceLevel == 'bulk') {
                return this.selectedStudiesIds.length > 0
            } else {
                return true;
            }
        },
        hasAddSeriesButton() {
            return this.studiesSourceType == SourceType.LOCAL_ORTHANC && 
                (this.uiOptions.EnableAddSeries && this.resourceLevel == 'study');
        },
        hasWsiButton() {
            if (this.studiesSourceType != SourceType.LOCAL_ORTHANC) {
                return false;
            }
            
            if (this.resourceLevel != 'series' || !this.hasWsiViewer) {
                return false;
            }
            return true;
        },
        isWsiButtonEnabled() {
            return this.isWsiSeries;
        },
        wsiViewerUrl() {
            return api.getWsiViewerUrl(this.resourceOrthancId);
        },
        hasSendToPeers() {
            return this.studiesSourceType == SourceType.LOCAL_ORTHANC && this.orthancPeers.length > 0;
        },
        hasSendToPeersWithTransfer() {
            return this.studiesSourceType == SourceType.LOCAL_ORTHANC && this.hasSendToPeers && ("transfers" in this.installedPlugins);
        },
        hasSendToDicomWeb() {
            return this.studiesSourceType == SourceType.LOCAL_ORTHANC && this.targetDicomWebServers.length > 0;
        },
        hasSendToDicomModalities() {
            return this.studiesSourceType == SourceType.LOCAL_ORTHANC && Object.keys(this.targetDicomModalities).length > 0;
        },
        hasOsimisViewer() {
            return this.studiesSourceType == SourceType.LOCAL_ORTHANC && "osimis-web-viewer" in this.installedPlugins;
        },
        osimisViewerUrl() {
            return api.getOsimisViewerUrl(this.resourceLevel, this.resourceOrthancId);
        },
        hasWsiViewer() {
            return this.studiesSourceType == SourceType.LOCAL_ORTHANC && "wsi" in this.installedPlugins;
        },
        hasStoneViewer() {
            return this.studiesSourceType == SourceType.LOCAL_ORTHANC && "stone-webviewer" in this.installedPlugins;
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
            return this.studiesSourceType == SourceType.LOCAL_ORTHANC && 
                this.hasStoneViewer && (this.resourceLevel == 'study' || this.resourceLevel == 'bulk');
        },
        isStoneViewerButtonEnabled() {
            return (this.resourceLevel == 'study' || (this.resourceLevel == 'bulk' && this.selectedStudiesIds.length > 0));
        },
        hasVolView() {
            return "volview" in this.installedPlugins;
        },
        volViewUrl() {
            return api.getVolViewUrl(this.resourceLevel, this.resourceOrthancId);
        },
        hasVolViewButton() {
            return this.studiesSourceType == SourceType.LOCAL_ORTHANC && this.hasVolView && (this.resourceLevel == 'study' || this.resourceLevel == 'series');
        },
        isVolViewButtonEnabled() {
            return (this.resourceLevel == 'study' || this.resourceLevel == 'series');
        },
        hasOhifViewer() {
            return this.uiOptions.EnableOpenInOhifViewer || this.uiOptions.EnableOpenInOhifViewer3;
        },
        hasOhifViewerButton() { // Basic viewer
            if (!this.uiOptions.ViewersOrdering.includes("ohif") || this.studiesSourceType != SourceType.LOCAL_ORTHANC) {
                return false;
            }

            // disable if it only contains non images modalities: 
            let modalities = this.modalitiesList;
            modalities = modalities.filter(x => !['SM', 'ECG', 'SR', 'SEG'].includes(x));  // since Set.difference is not supported on Firefox as of March 2024

            if (modalities.length == 0 && this.resourceLevel != "bulk") // we can not check the modalities list for bulk mode
            {
                return false;
            }

            if (this.uiOptions.EnableOpenInOhifViewer3) {
                return this.hasOhifViewer && (this.resourceLevel == 'study' || (this.resourceLevel == 'bulk' && this.ohifDataSource == 'dicom-web'));
            } else {
                return this.hasOhifViewer && this.resourceLevel == 'study';
            }
        },
        hasOhifViewerButtonVr() {
            if (!this.uiOptions.ViewersOrdering.includes("ohif-vr") || this.studiesSourceType != SourceType.LOCAL_ORTHANC) {
                return false;
            }

            // only for CT, PT and MR
            if (!(this.modalitiesList.includes("CT") || this.modalitiesList.includes("PT") || this.modalitiesList.includes("MR"))
                && this.resourceLevel != "bulk") // we can not check the modalities list for bulk mode)
            {
                return false;
            }

            if (this.uiOptions.EnableOpenInOhifViewer3) {
                return this.hasOhifViewer && (this.resourceLevel == 'study' || (this.resourceLevel == 'bulk' && this.ohifDataSource == 'dicom-web'));
            } else {
                return false;
            }
        },
        hasOhifViewerButtonTmtv() {
            if (!this.uiOptions.ViewersOrdering.includes("ohif-tmtv") || this.studiesSourceType != SourceType.LOCAL_ORTHANC) {
                return false;
            }

            // from isValidMode() in OHIF code
            if (!(this.modalitiesList.includes("CT") && this.modalitiesList.includes("PT") && !this.modalitiesList.includes("SM"))
                && this.resourceLevel != "bulk") // we can not check the modalities list for bulk mode)
            {
                return false;
            }

            if (this.uiOptions.EnableOpenInOhifViewer3) {
                return this.hasOhifViewer && (this.resourceLevel == 'study' || (this.resourceLevel == 'bulk' && this.ohifDataSource == 'dicom-web'));
            } else {
                return false;
            }
        },
        hasOhifViewerButtonSeg() {
            if (!this.uiOptions.ViewersOrdering.includes("ohif-seg") || this.studiesSourceType != SourceType.LOCAL_ORTHANC) {
                return false;
            }

            if (this.uiOptions.EnableOpenInOhifViewer3 && this.hasOhifViewer) {
                if (this.resourceLevel == "bulk" && this.ohifDataSource == 'dicom-web') {
                    return true; // unable to check the list of modalities in this case -> allow it
                } else if (this.resourceLevel == "study") {
                    // // from isValidMode() in OHIF code
                    // Don't show the mode if the selected studies have only one modality that is not supported by the mode
                    if (this.modalitiesList.length == 1 && ['SM', 'ECG', 'OT', 'DOC'].includes(this.modalitiesList[0])) {
                        return false;
                    } else {
                        return true;
                    }
                }
            }

            return false;
        },
        hasOhifViewerButtonMicroscopy() {
            if (!this.uiOptions.ViewersOrdering.includes("ohif-micro") || this.studiesSourceType != SourceType.LOCAL_ORTHANC) {
                return false;
            }
            
            // Must have at least one SM series
            if (!this.modalitiesList.includes("SM") && this.resourceLevel != "bulk") {// we can not check the modalities list for bulk mode)
                return false;
            }

            if (this.uiOptions.EnableOpenInOhifViewer3) {
                return this.hasOhifViewer && (this.resourceLevel == 'study' || (this.resourceLevel == 'bulk' && this.ohifDataSource == 'dicom-web'));
            } else {
                return false;
            }
        },
        ohifViewerUrl() {
            return this.getOhifViewerUrl('basic');
        },
        ohifViewerUrlVr() {
            return this.getOhifViewerUrl('vr');
        },
        ohifViewerUrlTmtv() {
            return this.getOhifViewerUrl('tmtv');
        },
        ohifViewerUrlSeg() {
            return this.getOhifViewerUrl('seg');
        },
        ohifViewerUrlMicro() {
            return this.getOhifViewerUrl('microscopy');
        },
        isOhifButtonBasicViewerEnabled() {
            if (this.uiOptions.EnableOpenInOhifViewer3) { // OHIF V3
                return (this.resourceLevel == 'study' || (this.resourceLevel == 'bulk' && this.selectedStudiesIds.length > 0));
            } else { // OHIF V2
                return this.resourceLevel == 'study';
            }
        },
        isOhifButtonVrEnabled() {
            if (this.uiOptions.EnableOpenInOhifViewer3) { // OHIF V3
                return (this.resourceLevel == 'study' || (this.resourceLevel == 'bulk' && this.selectedStudiesIds.length > 0));
            } else { // OHIF V2
                return false;
            }
        },
        isOhifButtonTmtvEnabled() {
            if (this.uiOptions.EnableOpenInOhifViewer3) { // OHIF V3
                return (this.resourceLevel == 'study' || (this.resourceLevel == 'bulk' && this.selectedStudiesIds.length > 0));
            } else { // OHIF V2
                return false;
            }
        },
        isOhifButtonMicroscopyEnabled() {
            if (this.uiOptions.EnableOpenInOhifViewer3) { // OHIF V3

                return (this.resourceLevel == 'study' || (this.resourceLevel == 'bulk' && this.selectedStudiesIds.length > 0));
            } else { // OHIF V2
                return false;
            }
        },
        isOhifButtonSegEnabled() {
            if (this.uiOptions.EnableOpenInOhifViewer3) { // OHIF V3
               return (this.resourceLevel == 'study' || (this.resourceLevel == 'bulk' && this.selectedStudiesIds.length > 0));
            } else { // OHIF V2
                return false;
            }
        },
        hasMedDreamViewer() {
            return this.studiesSourceType == SourceType.LOCAL_ORTHANC && this.uiOptions.EnableOpenInMedDreamViewer;
        },
        hasMedDreamViewerButton() {
            return this.hasMedDreamViewer && (this.resourceLevel == 'study' || this.resourceLevel == 'bulk');
        },
        isMedDreamViewerButtonEnabled() {
            return (this.resourceLevel == 'study' || (this.resourceLevel == 'bulk' && this.selectedStudiesIds.length > 0));
        },
        medDreamViewerUrl() {
            return this.uiOptions.MedDreamViewerPublicRoot + "?study=" + this.resourceDicomUid;
        },
        isPdfPreview() {
            if (this.studiesSourceType == SourceType.LOCAL_ORTHANC && this.resourceLevel == 'instance') {
                return this.instanceHeaders["0002,0002"]["Value"] == "1.2.840.10008.5.1.4.1.1.104.1";
            } else {
                return false;
            }
        },
        hasInstancePreviewButton() {
            return this.studiesSourceType == SourceType.LOCAL_ORTHANC && this.resourceLevel == 'instance';
        },
        instancePreviewUrl() {
            if (this.isPdfPreview) {
                return api.getInstancePdfUrl(this.resourceOrthancId);
            } else {
                return api.getInstancePreviewUrl(this.resourceOrthancId);
            }
        },
        hasInstanceDownloadButton() {
            return this.studiesSourceType == SourceType.LOCAL_ORTHANC && 
                this.uiOptions.EnableDownloadDicomFile && this.resourceLevel == 'instance';
        },
        instanceDownloadUrl() {
            let filename = null;
            if (this.resourceLevel == 'instance') {
                console.log(this.instanceTags);
                filename = resourceHelpers.replaceResourceTagsInString(this.uiOptions.DownloadInstanceFileNameTemplate, this.patientMainDicomTags, this.studyMainDicomTags, this.seriesMainDicomTags, this.instanceTags, this.resourceOrthancId);
            }
            return api.getInstanceDownloadUrl(this.resourceOrthancId, filename);
        },
        downloadBulkZipUrl() {
            return api.getBulkDownloadZipUrl(this.resourcesOrthancId);
        },
        downloadBulkDicomDirUrl() {
            return api.getBulkDownloadDicomDirUrl(this.resourcesOrthancId);
        },
        downloadZipUrl() {
            let filename = null;
            if (this.resourceLevel == 'study' || this.resourceLevel == 'series') {
                if (this.resourceLevel == 'study') {
                    filename = resourceHelpers.replaceResourceTagsInString(this.uiOptions.DownloadStudyFileNameTemplate, this.patientMainDicomTags, this.studyMainDicomTags, this.seriesMainDicomTags, this.instanceTags, this.resourceOrthancId);
                } else if (this.resourceLevel == 'series') {
                    filename = resourceHelpers.replaceResourceTagsInString(this.uiOptions.DownloadSeriesFileNameTemplate, this.patientMainDicomTags, this.studyMainDicomTags, this.seriesMainDicomTags, this.instanceTags, this.resourceOrthancId);
                }
            }
            
            return api.getDownloadZipUrl(this.resourceLevel, this.resourceOrthancId, filename);
        },
        downloadDicomDirUrl() {
            let filename = null;
            if (this.resourceLevel == 'study' || this.resourceLevel == 'series') {
                if (this.resourceLevel == 'study') {
                    filename = resourceHelpers.replaceResourceTagsInString(this.uiOptions.DownloadStudyFileNameTemplate, this.patientMainDicomTags, this.studyMainDicomTags, this.seriesMainDicomTags, this.instanceTags, this.resourceOrthancId);
                } else if (this.resourceLevel == 'series') {
                    filename = resourceHelpers.replaceResourceTagsInString(this.uiOptions.DownloadSeriesFileNameTemplate, this.patientMainDicomTags, this.studyMainDicomTags, this.seriesMainDicomTags, this.instanceTags, this.resourceOrthancId);
                }
            }
            return api.getDownloadDicomDirUrl(this.resourceLevel, this.resourceOrthancId, filename);
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
        volViewIcon() {
            return this.getViewerIcon("volview");
        },
        medDreamViewerIcon() {
            return this.getViewerIcon("meddream");
        },
        ohifViewerIcon() {
            return this.getViewerIcon("ohif");
        },
        ohifViewerIconVr() {
            return this.getViewerIcon("ohif-vr");
        },
        ohifViewerIconTmtv() {
            return this.getViewerIcon("ohif-tmtv");
        },
        ohifViewerIconSeg() {
            return this.getViewerIcon("ohif-seg");
        },
        ohifViewerIconMicro() {
            return this.getViewerIcon("ohif-micro");
        },
        wsiViewerIcon() {
            return this.getViewerIcon("wsi");
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
        hasDownloadZipButton() {
            return this.studiesSourceType == SourceType.LOCAL_ORTHANC && this.uiOptions.EnableDownloadZip && this.resourceLevel != 'instance' && this.resourceLevel != 'bulk';
        },
        hasBulkDownloadZipButton() {
            if (this.system.ApiVersion < 22) // the /tools/create-archive GET route has been introduced in 1.12.2
            {
                return false;
            }
            return this.studiesSourceType == SourceType.LOCAL_ORTHANC && this.uiOptions.EnableDownloadZip && this.resourceLevel == 'bulk'
        },
        isBulkDownloadZipEnabled() {
            return this.selectedStudiesIds.length > 0;
        },
        hasDownloadDicomDirButton() {
            return this.studiesSourceType == SourceType.LOCAL_ORTHANC && this.uiOptions.EnableDownloadDicomDir && this.resourceLevel != 'instance' && this.resourceLevel != 'bulk';
        },
        hasBulkDownloadDicomDirButton() {
            if (this.system.ApiVersion < 22) // the /tools/create-media GET route has been introduced in 1.12.2
            {
                return false;
            }
            return this.studiesSourceType == SourceType.LOCAL_ORTHANC && this.uiOptions.EnableDownloadDicomDir && this.resourceLevel == 'bulk'
        },
        isBulkDownloadDicomDirEnabled() {
            return this.selectedStudiesIds.length > 0;
        },
        hasRetrieveButton() {
            return this.studiesSourceType == SourceType.REMOTE_DICOM || this.studiesSourceType == SourceType.REMOTE_DICOM_WEB;
        },
        isRetrieveButtonEnabled() {
            return (this.resourceLevel != 'bulk' || (this.resourceLevel == 'bulk' && this.selectedStudiesIds.length > 0));
        },
        hasAnonymizationButton() {
            return this.studiesSourceType == SourceType.LOCAL_ORTHANC && this.resourceLevel != 'bulk';
        },
        isAnonymizationEnabled() {
            if (this.resourceLevel == "study" || this.resourceLevel == "series") {
                return this.uiOptions.EnableAnonymization;
            } else {
                return false;
            }
        },
        hasModificationButton() {
            return this.studiesSourceType == SourceType.LOCAL_ORTHANC && this.resourceLevel != 'bulk';
        },
        isModificationEnabled() {
            if (this.resourceLevel == "study" || this.resourceLevel == "series") {
                return this.uiOptions.EnableModification;
            } else {
                return false;
            }
        },
        hasApiViewButton() {
            return  this.resourceLevel != 'bulk' && this.studiesSourceType == SourceType.LOCAL_ORTHANC && this.uiOptions.EnableApiViewMenu;
        },
        resourcesOrthancId() {
            if (this.resourceLevel == 'bulk') {
                return this.selectedStudiesIds;
            } else {
                return [this.resourceOrthancId];
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
        },
        hasCustomButtons() {
            return this.uiOptions.CustomButtons && this.uiOptions.CustomButtons[this.resourceLevel]
                && this.uiOptions.CustomButtons[this.resourceLevel].length > 0;
        },
        CustomButtons() {
            let customButtons = [];
            for (const customButton of this.uiOptions.CustomButtons[this.resourceLevel]) {
                let cloneCustomButton = Object.assign({}, customButton);
                cloneCustomButton._computedLinkUrl = resourceHelpers.replaceResourceTagsInString(customButton.Url, this.patientMainDicomTags, this.studyMainDicomTags, this.seriesMainDicomTags, this.instanceTags, this.resourceOrthancId);
                customButtons.push(cloneCustomButton);
            }

            return customButtons;
        }
    },
    components: { Modal, ShareModal, ModifyModal, TokenLinkButton, BulkLabelsModal, AddSeriesModal }
}
</script>

<template>
    <div>
        <div class="btn-group">
            <span v-for="viewer in uiOptions.ViewersOrdering" :key="viewer">
                <TokenLinkButton v-if="viewer == 'meddream' && hasMedDreamViewerButton"
                    :disabled="!isMedDreamViewerButtonEnabled"
                    :iconClass="medDreamViewerIcon" :level="computedResourceLevel" :linkUrl="medDreamViewerUrl"
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

                <TokenLinkButton v-if="viewer == 'volview' && hasVolViewButton"
                    :disabled="!isVolViewButtonEnabled"
                    :iconClass="volViewIcon" :level="computedResourceLevel" :linkUrl="volViewUrl"
                    :resourcesOrthancId="resourcesOrthancId" :title="$t('view_in_volview')"
                    :tokenType="'viewer-instant-link'" :opensInNewTab="true">
                </TokenLinkButton>

                <TokenLinkButton v-if="viewer == 'ohif' && hasOhifViewerButton"
                    :disabled="!isOhifButtonBasicViewerEnabled"
                    :iconClass="ohifViewerIcon" :level="computedResourceLevel" :linkUrl="ohifViewerUrl"
                    :resourcesOrthancId="resourcesOrthancId" :title="$t('view_in_ohif')"
                    :tokenType="'viewer-instant-link'" :opensInNewTab="true">
                </TokenLinkButton>

                <TokenLinkButton v-if="viewer == 'ohif-vr' && hasOhifViewerButtonVr"
                    :disabled="!isOhifButtonVrEnabled"
                    :iconClass="ohifViewerIconVr" :level="computedResourceLevel" :linkUrl="ohifViewerUrlVr"
                    :resourcesOrthancId="resourcesOrthancId" :title="$t('view_in_ohif_vr')"
                    :tokenType="'viewer-instant-link'" :opensInNewTab="true">
                </TokenLinkButton>

                <TokenLinkButton v-if="viewer == 'ohif-tmtv' && hasOhifViewerButtonTmtv"
                    :disabled="!isOhifButtonTmtvEnabled"
                    :iconClass="ohifViewerIconTmtv" :level="computedResourceLevel" :linkUrl="ohifViewerUrlTmtv"
                    :resourcesOrthancId="resourcesOrthancId" :title="$t('view_in_ohif_tmtv')"
                    :tokenType="'viewer-instant-link'" :opensInNewTab="true">
                </TokenLinkButton>

                <TokenLinkButton v-if="viewer == 'ohif-seg' && hasOhifViewerButtonSeg"
                    :disabled="!isOhifButtonSegEnabled"
                    :iconClass="ohifViewerIconSeg" :level="computedResourceLevel" :linkUrl="ohifViewerUrlSeg"
                    :resourcesOrthancId="resourcesOrthancId" :title="$t('view_in_ohif_seg')"
                    :tokenType="'viewer-instant-link'" :opensInNewTab="true">
                </TokenLinkButton>

                <TokenLinkButton v-if="viewer == 'ohif-micro' && hasOhifViewerButtonMicroscopy"
                    :disabled="!isOhifButtonMicroscopyEnabled"
                    :iconClass="ohifViewerIconMicro" :level="computedResourceLevel" :linkUrl="ohifViewerUrlMicro"
                    :resourcesOrthancId="resourcesOrthancId" :title="$t('view_in_ohif_microscopy')"
                    :tokenType="'viewer-instant-link'" :opensInNewTab="true">
                </TokenLinkButton>
            </span>
            <TokenLinkButton v-if="hasInstancePreviewButton"
                :iconClass="'bi bi-binoculars'" :level="this.resourceLevel" :linkUrl="instancePreviewUrl"
                :resourcesOrthancId="[resourceOrthancId]" :title="$t('preview')"
                :tokenType="'download-instant-link'" :opensInNewTab="true">
            </TokenLinkButton>
            <TokenLinkButton v-if="hasWsiButton"
                :hidden="!isWsiButtonEnabled"
                :iconClass="wsiViewerIcon" :level="this.resourceLevel" :linkUrl="wsiViewerUrl"
                :resourcesOrthancId="resourcesOrthancId" :title="$t('view_in_wsi_viewer')"
                :tokenType="'viewer-instant-link'" :opensInNewTab="true">
            </TokenLinkButton>
        </div>
        <div class="btn-group">
            <TokenLinkButton v-if="hasDownloadZipButton"
                :iconClass="'bi bi-download'" :level="this.resourceLevel" :linkUrl="downloadZipUrl"
                :resourcesOrthancId="resourcesOrthancId" :title="$t('download_zip')" :tokenType="'download-instant-link'">
            </TokenLinkButton>
            <TokenLinkButton v-if="hasBulkDownloadZipButton"
                :iconClass="'bi bi-download'" :level="'bulk-study'" :linkUrl="downloadBulkZipUrl"
                :resourcesOrthancId="resourcesOrthancId" :title="$t('download_zip')" :tokenType="'download-instant-link'"
                :disabled="!isBulkDownloadZipEnabled">
            </TokenLinkButton>
            <TokenLinkButton v-if="hasDownloadDicomDirButton"
                :iconClass="'bi bi-box-arrow-down'" :level="this.resourceLevel" :linkUrl="downloadDicomDirUrl"
                :resourcesOrthancId="[resourceOrthancId]" :title="$t('download_dicomdir')"
                :tokenType="'download-instant-link'">
            </TokenLinkButton>
            <TokenLinkButton v-if="hasBulkDownloadDicomDirButton"
                :iconClass="'bi bi-box-arrow-down'" :level="'bulk-study'" :linkUrl="downloadBulkDicomDirUrl"
                :resourcesOrthancId="resourcesOrthancId" :title="$t('download_dicomdir')" :tokenType="'download-instant-link'"
                :disabled="!isBulkDownloadDicomDirEnabled">
            </TokenLinkButton>
            <TokenLinkButton v-if="hasInstanceDownloadButton"
                :iconClass="'bi bi-download'" :level="this.resourceLevel" :linkUrl="instanceDownloadUrl"
                :resourcesOrthancId="[resourceOrthancId]" :title="$t('download_dicom_file')"
                :tokenType="'download-instant-link'">
            </TokenLinkButton>
        </div>
        <div class="btn-group" >
            <button v-if="hasDeleteButton" class="btn btn-sm btn-secondary m-1" type="button"
                data-bs-toggle="modal" v-bind:data-bs-target="'#delete-modal-' + this.resourceOrthancId" :disabled="!isDeleteEnabled">
                <i class="bi bi-trash" data-bs-toggle="tooltip" :title="$t('delete')" ></i>
            </button>
            <Modal v-if="hasDeleteButton" :id="'delete-modal-' + this.resourceOrthancId"
                :headerText="$t(this.deleteResourceTitle) + ' ' + this.resourceTitle" :okText="$t('delete')"
                :cancelText="$t('cancel')" :bodyText="$t(this.deleteResourceBody)" @ok="deleteResource($event)">
            </Modal>
        </div>
        <div v-if="hasShareButton" class="btn-group">
            <button class="btn btn-sm btn-secondary m-1" type="button" data-bs-toggle="modal"
                v-bind:data-bs-target="'#share-modal-' + this.resourceOrthancId" :disabled="!isShareEnabled">
                <i class="bi bi-share" data-bs-toggle="tooltip" :title="$t('share.button_title')"></i>
            </button>
            <ShareModal :id="'share-modal-' + this.resourceOrthancId"
                :orthancId="this.resourceOrthancId" :studyMainDicomTags="this.studyMainDicomTags"
                :patientMainDicomTags="this.patientMainDicomTags"></ShareModal>
        </div>
        <div v-if="hasModificationButton" class="btn-group">
            <button v-if="isModificationEnabled" class="btn btn-sm btn-secondary m-1" type="button" data-bs-toggle="modal"
                v-bind:data-bs-target="'#modify-modal-' + this.resourceOrthancId">
                <i class="bi bi-pencil" data-bs-toggle="tooltip" :title="$t('modify.modify_button_title')"></i>
            </button>
            <ModifyModal v-if="isModificationEnabled" :id="'modify-modal-' + this.resourceOrthancId"
                :orthancId="this.resourceOrthancId" :resourceLevel="this.resourceLevel"
                :seriesMainDicomTags="this.seriesMainDicomTags" :studyMainDicomTags="this.studyMainDicomTags"
                :patientMainDicomTags="this.patientMainDicomTags" :isAnonymization="false"></ModifyModal>
        </div>
        <div v-if="hasAnonymizationButton" class="btn-group">
            <button v-if="isAnonymizationEnabled" class="btn btn-sm btn-secondary m-1" type="button" data-bs-toggle="modal"
                v-bind:data-bs-target="'#anonymize-modal-' + this.resourceOrthancId">
                <i class="bi bi-person-slash" data-bs-toggle="tooltip" :title="$t('modify.anonymize_button_title')"></i>
            </button>
            <ModifyModal v-if="isAnonymizationEnabled" :id="'anonymize-modal-' + this.resourceOrthancId"
                :orthancId="this.resourceOrthancId" :resourceLevel="this.resourceLevel"
                :seriesMainDicomTags="this.seriesMainDicomTags" :studyMainDicomTags="this.studyMainDicomTags"
                :patientMainDicomTags="this.patientMainDicomTags" :isAnonymization="true"></ModifyModal>
        </div>
        <div v-if="hasAddSeriesButton" class="btn-group" >
            <button class="btn btn-sm btn-secondary m-1" type="button" data-bs-toggle="modal"
                v-bind:data-bs-target="'#add-series-modal-' + this.resourceOrthancId">
                <i class="bi bi-file-earmark-plus" data-bs-toggle="tooltip" :title="$t('add_series.button_title')"></i>
            </button>
            <AddSeriesModal :id="'add-series-modal-' + this.resourceOrthancId"
                :orthancStudyId="this.resourceOrthancId" :studyMainDicomTags="this.studyMainDicomTags"
                :patientMainDicomTags="this.patientMainDicomTags"></AddSeriesModal>
        </div>


        <div class="btn-group" v-if="hasApiViewButton">
            <div class="dropdown">
                <button class="dropdown btn btn-sm btn-secondary m-1 dropdown-toggle"
                    type="button" id="apiDropdownMenuId" data-bs-toggle="dropdown" aria-expanded="false">
                    <span data-bs-toggle="tooltip" title="API">
                        <i class="bi bi-code-slash"></i>
                    </span>
                </button>
                <ul class="dropdown-menu" aria-labelledby="apiDropdownMenuId">
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
        <div class="btn-group" v-if="this.resourceLevel == 'bulk'">
            <!-- <button v-if="hasLabelsButton" class="btn btn-sm btn-secondary m-1" type="button"
                data-bs-toggle="modal" v-bind:data-bs-target="'#labels2-modal2-' + this.componentId" :disabled="!isLabelsEnabled">
                <i class="bi bi-tag" data-bs-toggle="tooltip" :title="$t('labels.edit_labels_button')" ></i>
            </button> -->
            <button v-if="hasLabelsButton" class="btn btn-sm btn-secondary m-1" type="button"
                :disabled="!isLabelsEnabled" @click="showBulkLabelModal()">
                <i class="bi bi-tag" data-bs-toggle="tooltip" :title="$t('labels.edit_labels_button')" ></i>
            </button>
            <BulkLabelsModal v-if="uiOptions.EnableEditLabels && isBulkLabelModalVisible" :id="'labels2-modal2-' + this.componentId" ref="bulk-label-modal"
            :resourceLevel="this.resourceLevel" :resourcesOrthancId="selectedStudiesIds" @bulkModalClosed="onBulkModalClosed">

            </BulkLabelsModal>
        </div>
        <div class="btn-group">
            <div class="dropdown">
                <button v-if="hasSendTo" class="dropdown btn btn-sm btn-secondary m-1 dropdown-toggle" type="button"
                    id="sendToDropdownMenuId" data-bs-toggle="dropdown" aria-expanded="false" :disabled="!isSendToEnabled">
                    <span data-bs-toggle="tooltip" :title="$t('send_to.button_title')">
                        <i class="bi bi-send"></i>
                    </span>
                </button>
                <ul class="dropdown-menu" aria-labelledby="sendToDropdownMenuId" v-if="hasSendTo">
                    <li v-if="hasSendToPeers" class="dropdown-submenu">
                        <a class="dropdown-item" @click="toggleSubMenu" href="#">
                            {{ $t('send_to.orthanc_peer') }}
                            <i class="bi bi-caret-down"></i>
                        </a>
                        <ul class="dropdown-menu">
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
                        <ul class="dropdown-menu">
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
                        <ul class="dropdown-menu">
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
                        <ul class="dropdown-menu">
                            <li v-for="modality in Object.keys(targetDicomModalities)" :key="modality">
                                <a class="dropdown-item" @click="sendToDicomModality(modality)">{{ modality }}</a>
                            </li>
                        </ul>
                    </li>
                </ul>
            </div>
        </div>
        <div v-if="hasRetrieveButton" class="btn-group" >
            <button class="btn btn-sm btn-secondary m-1" type="button"
                :disabled="!isRetrieveButtonEnabled" @click="retrieve">
                <i class="bi bi-box-arrow-in-down" data-bs-toggle="tooltip" :title="$t('retrieve')" ></i>
            </button>
        </div>
        <div v-if="hasCustomButtons" class="btn-group">
            <div v-for="customButton in CustomButtons" :key="customButton.Id" class>
                <a v-if="customButton.HttpMethod=='GET'"
                    class="btn btn-sm m-1 btn-secondary" type="button"
                    data-bs-toggle="tooltip" :title="customButton.Tooltip" :target="customButton.Target" :href="customButton._computedLinkUrl">
                    <i :class="customButton.Icon"></i>
                </a>
                <a v-if="customButton.HttpMethod=='PUT' || customButton.HttpMethod=='POST'"
                    class="btn btn-sm m-1 btn-secondary" type="button"
                    data-bs-toggle="tooltip" :title="customButton.Tooltip" @click="customButtonClicked(customButton)">
                    <i :class="customButton.Icon"></i>
                </a>
            </div>
        </div>
    </div>
</template>

<style>

.dropdown-submenu {
    position: relative;
}

.dropdown-submenu .dropdown-menu {
    top: 0;
    right: 100%;
    margin-top: -1px;
}

.fa-button {
    line-height: 1.5;
}
</style>