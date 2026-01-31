<script>
import Modal from "./Modal.vue";
import ShareModal from "./ShareModal.vue";
import ModifyModal from "./ModifyModal.vue";
import AddSeriesModal from "./AddSeriesModal.vue";
import $ from "jquery";
import { mapState } from "vuex";
import api from "../orthancApi";
import resourceHelpers from "../helpers/resource-helpers";
import clipboardHelpers from "../helpers/clipboard-helpers";
import TokenLinkButton from "./TokenLinkButton.vue";
import BulkLabelsModal from "./BulkLabelsModal.vue";
import SourceType from "../helpers/source-type";
import CreateWorklistModal from "./CreateWorklistModal.vue";
import axios from "axios";

export default {
  props: [
    "resourceOrthancId",
    "resourceDicomUid",
    "resourceLevel",
    "customClass",
    "seriesMainDicomTags",
    "studyMainDicomTags",
    "patientMainDicomTags",
    "instanceTags",
    "instanceHeaders",
    "studySeries",
    "seriesInstances",
    "smallIcons",
  ],
  setup() {
    return {};
  },

  data() {
    return {
      isBulkLabelModalVisible: false,
      isWsiSeries: false,
      modalitiesList: [],
      isNiftiCompatible: false,
    };
  },
  watch: {
    async studySeries(newValue, oldValue) {
      if (this.resourceLevel == "study") {
        // build the modalitiesList to enable/disable viewers
        let modalitiesSet = new Set();
        for (let seriesDetail of this.studySeries) {
          modalitiesSet.add(seriesDetail["MainDicomTags"]["Modality"]);
        }
        this.modalitiesList = Array.from(modalitiesSet);
      } else if (this.resourceLevel == "bulk") {
        // build the modalitiesList to enable/disable viewers
        let modalitiesSet = new Set();
        for (let selectedStudyId of this.selectedStudiesIds) {
          let seriesDetails = await api.getStudySeries(selectedStudyId);
          for (let seriesDetail of seriesDetails) {
            modalitiesSet.add(seriesDetail["MainDicomTags"]["Modality"]);
          }
        }
        this.modalitiesList = Array.from(modalitiesSet);
      }
    },
    async seriesInstances(newValue, oldValue) {
      if (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        this.resourceLevel == "series"
      ) {
        let firstInstanceTags = await api.getSimplifiedInstanceTags(
          this.seriesInstances[0]["ID"],
        );
        this.modalitiesList = [firstInstanceTags["Modality"]];
        this.isWsiSeries =
          firstInstanceTags["SOPClassUID"] ==
            "1.2.840.10008.5.1.4.1.1.77.1.6" ||
          firstInstanceTags["Modality"] == "SM";
      }
    },
  },

  async mounted() {
    // check if the image is NIfTI compatible: make sure this is a 3D image
    if (!this.isPluginEnabled("neuro")) {
      return;
    }

    if (this.resourceLevel != "series" && this.resourceLevel != "instance") {
      return;
    }
    let instanceTags = null;
    if (this.resourceLevel == "series") {
      const instances = await api.getSeriesInstances(this.resourceOrthancId);
      instanceTags = await api.getSimplifiedInstanceTags(instances[0]["ID"]);
    } else if (this.resourceLevel == "instance") {
      instanceTags = await api.getSimplifiedInstanceTags(
        this.resourceOrthancId,
      );
    }

    if (
      "PixelSpacingBetweenSlices" in instanceTags ||
      "SliceThickness" in instanceTags
    ) {
      this.isNiftiCompatible = true;
    } else {
      console.info(
        "no PixelSpacingBetweenSlices or SliceThickness -> not possible to export to nifti",
      );
      this.isNiftiCompatible = false;
    }
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
      if (this.resourceLevel == "bulk") {
        api
          .deleteResources(this.resourcesOrthancId)
          .then(() => {
            window.location.reload();
          })
          .catch((reason) => {
            console.error(
              "failed to delete resources : ",
              this.resourceOrthancId,
              reason,
            );
          });
      } else {
        api
          .deleteResource(this.resourceLevel, this.resourceOrthancId)
          .then(() => {
            this.$emit("deletedResource");
          })
          .catch((reason) => {
            console.error(
              "failed to delete resource : ",
              this.resourceOrthancId,
              reason,
            );
          });
      }
    },
    getApiUrl(subRoute) {
      return api.getApiUrl(
        this.resourceLevel,
        this.resourceOrthancId,
        subRoute,
      );
    },
    async sendToDicomWebServer(server) {
      const jobId = await api.sendToDicomWebServer(
        this.resourcesOrthancId,
        server,
      );
      this.$store.dispatch("jobs/addJob", {
        jobId: jobId,
        name: "Send to DicomWeb (" + server + ")",
      });
    },
    async sendToOrthancPeer(peer) {
      const jobId = await api.sendToOrthancPeer(this.resourcesOrthancId, peer);
      this.$store.dispatch("jobs/addJob", {
        jobId: jobId,
        name: "Send to Peer (" + peer + ")",
      });
    },
    async sendToOrthancPeerWithTransfers(peer) {
      const jobId = await api.sendToOrthancPeerWithTransfers(
        this.resourcesForTransfer,
        peer,
      );
      this.$store.dispatch("jobs/addJob", {
        jobId: jobId,
        name: "Transfer to Peer (" + peer + ")",
      });
    },
    async sendToDicomModality(modality) {
      const jobId = await api.sendToDicomModality(
        this.resourcesOrthancId,
        modality,
      );
      this.$store.dispatch("jobs/addJob", {
        jobId: jobId,
        name: "Send to DICOM (" + modality + ")",
      });
    },
    async retrieve() {
      if (this.studiesSourceType == SourceType.REMOTE_DICOM) {
        if (this.resourceLevel == "bulk") {
          let moveQueries = this.selectedStudies.map((s) => {
            return {
              StudyInstanceUID: s["ID"],
              PatientID: s["PatientMainDicomTags"]["PatientID"],
            };
          });

          const jobId = await api.remoteDicomRetrieveResources(
            "Study",
            this.studiesRemoteSource,
            moveQueries,
          );
          this.$store.dispatch("jobs/addJob", {
            jobId: jobId,
            name:
              "Retrieve " +
              this.selectedStudies.length +
              " studies from (" +
              this.studiesRemoteSource +
              ")",
          });
        } else {
          let moveQuery = {
            StudyInstanceUID: this.studyMainDicomTags.StudyInstanceUID,
            PatientID: this.patientMainDicomTags.PatientID,
          };

          if (
            this.resourceLevel == "series" ||
            this.resourceLevel == "instance"
          ) {
            moveQuery["SeriesInstanceUID"] =
              this.seriesMainDicomTags.SeriesInstanceUID;
          }
          if (this.resourceLevel == "instance") {
            moveQuery["SOPInstanceUID"] = this.instanceTags["0008,0018"].Value;
          }

          const jobId = await api.remoteDicomRetrieveResource(
            this.capitalizeFirstLetter(this.resourceLevel),
            this.studiesRemoteSource,
            moveQuery,
          );
          this.$store.dispatch("jobs/addJob", {
            jobId: jobId,
            name:
              "Retrieve " +
              this.capitalizeFirstLetter(this.resourceLevel) +
              " from (" +
              this.studiesRemoteSource +
              ")",
          });
        }
      } else if (this.studiesSourceType == SourceType.REMOTE_DICOM_WEB) {
        let resources;
        if (this.resourceLevel == "bulk") {
          resources = this.selectedStudies.map((s) => {
            return { Study: s["ID"] };
          });
        } else if (this.resourceLevel == "study") {
          resources = [{ Study: this.studyMainDicomTags.StudyInstanceUID }];
        } else if (this.resourceLevel == "series") {
          resources = [
            {
              Study: this.studyMainDicomTags.StudyInstanceUID,
              Series: this.seriesMainDicomTags.SeriesInstanceUID,
            },
          ];
        } else if (this.resourceLevel == "instance") {
          resources = [
            {
              Study: this.studyMainDicomTags.StudyInstanceUID,
              Series: this.seriesMainDicomTags.SeriesInstanceUID,
              Instance: this.instanceTags.SOPInstanceUID,
            },
          ];
        }
        const jobId = await api.wadoRsRetrieve(
          this.studiesRemoteSource,
          resources,
        );
        this.$store.dispatch("jobs/addJob", {
          jobId: jobId,
          name:
            "Retrieve " +
            this.capitalizeFirstLetter(this.resourceLevel) +
            " from (" +
            this.studiesRemoteSource +
            ")",
        });
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

          if (
            this.hasOhifViewer &&
            ["ohif", "ohif-vr", "ohif-tmtv", "ohif-seg", "ohif-micro"].includes(
              forViewer,
            )
          ) {
            return viewersIcons[viewer];
          }

          if (this.hasMedDreamViewer && forViewer == "meddream") {
            return viewersIcons[viewer];
          }

          if (this.hasWsiViewer && forViewer == "wsi") {
            return viewersIcons[viewer];
          }

          if (this.hasStlViewer && forViewer == "stl") {
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
        this.$refs["bulk-label-modal"].showModal();
      }, 50);
    },
    async onBulkModalClosed() {
      this.isBulkLabelModalVisible = false;
      await this.$store.dispatch("labels/refresh");
    },
    getOhifViewerUrl(mode) {
      if (this.resourceLevel == "bulk") {
        const selectedStudiesDicomIds = this.selectedStudies.map(
          (s) => s["MainDicomTags"]["StudyInstanceUID"],
        );
        const url = api.getOhifViewerUrlForDicomWebBulkStudies(
          mode,
          selectedStudiesDicomIds,
        );
        return url;
      } else {
        if (this.ohifDataSource == "dicom-web") {
          return api.getOhifViewerUrlForDicomWeb(mode, this.resourceDicomUid);
        } else if (this.ohifDataSource == "dicom-json") {
          return api.getOhifViewerUrlForDicomJson(mode, this.resourceOrthancId);
        }
      }
    },
    async customButtonClicked(customButton) {
      let url = await resourceHelpers.replaceResourceTagsInStringWithTokens(
        customButton.Url,
        this.patientMainDicomTags,
        this.studyMainDicomTags,
        this.seriesMainDicomTags,
        this.instanceTags,
        this.resourceOrthancId,
        this.resourceLevel,
        this.selectedStudiesIds,
        this.selectedStudiesDicomIds,
      );
      let response;

      if (customButton.HttpMethod == "GET") {
        let link = document.querySelector("#hidden-link-for-custom-buttons");
        if (link == null) {
          link = document.createElement("a");
          document.body.appendChild(link);
          link.setAttribute("id", "hidden-link-for-custom-buttons");
          link.setAttribute("type", "hidden");
        }
        link.target = this.target;
        link.href = url;
        link.click();
      } else if (
        customButton.HttpMethod == "PUT" ||
        customButton.HttpMethod == "POST"
      ) {
        let payload = await resourceHelpers.replaceResourceTagsInJson(
          customButton.Payload,
          this.patientMainDicomTags,
          this.studyMainDicomTags,
          this.seriesMainDicomTags,
          this.instanceTags,
          this.resourceOrthancId,
          this.resourceLevel,
          this.selectedStudiesIds,
          this.selectedStudiesDicomIds,
        );

        if (customButton.HttpMethod == "PUT") {
          response = axios.put(url, payload);
        } else if (customButton.HttpMethod == "POST") {
          response = axios.post(url, payload);
        }
      } else if (customButton.HttpMethod == "DELETE") {
        response = axios.delete(url);
      }

      if (customButton.StartMessage) {
        this.messageBus.emit("show-toast", customButton.StartMessage);
      }

      await response;
      if (customButton.Refresh) {
        await this.$store.dispatch("labels/refresh");
        if (this.resourceLevel == "study") {
          const study = await api.getStudy(this.resourceOrthancId);
          await this.$store.dispatch("studies/reloadStudy", {
            studyId: this.resourceOrthancId,
            study: study,
          });
          this.messageBus.emit(
            "study-updated-" + this.resourceOrthancId,
            study,
          );
        } else {
          console.warn(
            "Refresh " + this.resourceLevel + " currently not supported",
          );
        }
      }
    },
    isPluginEnabled(pluginName) {
      return (
        pluginName in this.installedPlugins &&
        this.installedPlugins[pluginName].Enabled
      );
    },
  },
  computed: {
    ...mapState({
      uiOptions: (state) => state.configuration.uiOptions,
      studiesSourceType: (state) => state.studies.sourceType,
      studiesRemoteSource: (state) => state.studies.remoteSource,
      ohifDataSource: (state) => state.configuration.ohifDataSource,
      installedPlugins: (state) => state.configuration.installedPlugins,
      targetDicomWebServers: (state) =>
        state.configuration.targetDicomWebServers,
      targetDicomModalities: (state) =>
        state.configuration.targetDicomModalities,
      selectedStudiesIds: (state) => state.studies.selectedStudiesIds,
      selectedStudies: (state) => state.studies.selectedStudies,
      orthancPeers: (state) => state.configuration.orthancPeers,
      tokens: (state) => state.configuration.tokens,
      system: (state) => state.configuration.system,
    }),
    componentId() {
      if (this.resourceLevel == "bulk") {
        return "bulk-id";
      } else {
        return this.resourceOrthancId;
      }
    },
    hasSendTo() {
      return (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        this.uiOptions.EnableSendTo &&
        (this.hasSendToDicomWeb ||
          this.hasSendToPeers ||
          this.hasSendToDicomModalities ||
          this.hasSendToPeersWithTransfer)
      );
    },
    isSendToEnabled() {
      if (this.resourceLevel == "bulk") {
        return this.selectedStudiesIds.length > 0;
      } else {
        return true;
      }
    },
    hasLabelsButton() {
      return (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        this.uiOptions.EnableEditLabels &&
        this.resourceLevel == "bulk"
      );
    },
    isLabelsEnabled() {
      if (this.resourceLevel == "bulk") {
        return this.selectedStudiesIds.length > 0;
      } else {
        return false;
      }
    },
    hasDeleteButton() {
      return (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        this.uiOptions.EnableDeleteResources
      );
    },
    isDeleteEnabled() {
      if (this.resourceLevel == "bulk") {
        return this.selectedStudiesIds.length > 0;
      } else {
        return true;
      }
    },
    hasShareButton() {
      return (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        this.uiOptions.EnableShares &&
        this.isPluginEnabled("authorization") &&
        ["bulk", "study"].includes(this.resourceLevel)
      );
    },
    isShareEnabled() {
      if (this.resourceLevel == "bulk") {
        return this.selectedStudiesIds.length > 0;
      } else {
        return true;
      }
    },
    hasAddSeriesButton() {
      return (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        this.uiOptions.EnableAddSeries &&
        this.resourceLevel == "study"
      );
    },
    hasWsiButton() {
      if (this.studiesSourceType != SourceType.LOCAL_ORTHANC) {
        return false;
      }

      if (this.resourceLevel != "series" || !this.hasWsiViewer) {
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
      return (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        this.orthancPeers.length > 0
      );
    },
    hasSendToPeersWithTransfer() {
      return (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        this.hasSendToPeers &&
        this.isPluginEnabled("transfers")
      );
    },
    hasSendToDicomWeb() {
      return (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        this.targetDicomWebServers.length > 0
      );
    },
    hasSendToDicomModalities() {
      return (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        Object.keys(this.targetDicomModalities).length > 0
      );
    },
    hasOsimisViewer() {
      return (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        this.isPluginEnabled("osimis-web-viewer")
      );
    },
    osimisViewerUrl() {
      return api.getOsimisViewerUrl(this.resourceLevel, this.resourceOrthancId);
    },
    hasWsiViewer() {
      return (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        this.isPluginEnabled("wsi")
      );
    },
    hasStoneViewer() {
      return (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        this.isPluginEnabled("stone-webviewer")
      );
    },
    stoneViewerUrl() {
      if (this.resourceLevel == "bulk") {
        const selectedStudiesDicomIds = this.selectedStudies.map(
          (s) => s["MainDicomTags"]["StudyInstanceUID"],
        );
        const url = api.getStoneViewerUrlForBulkStudies(
          selectedStudiesDicomIds,
        );
        return url;
      } else {
        return api.getStoneViewerUrl(this.resourceLevel, this.resourceDicomUid);
      }
    },
    hasStoneViewerButton() {
      return (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        this.hasStoneViewer &&
        (this.resourceLevel == "study" || this.resourceLevel == "bulk")
      );
    },
    isStoneViewerButtonEnabled() {
      return (
        this.resourceLevel == "study" ||
        (this.resourceLevel == "bulk" && this.selectedStudiesIds.length > 0)
      );
    },
    hasVolView() {
      return this.isPluginEnabled("volview");
    },
    volViewUrl() {
      return api.getVolViewUrl(this.resourceLevel, this.resourceOrthancId);
    },
    hasVolViewButton() {
      return (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        this.hasVolView &&
        (this.resourceLevel == "study" || this.resourceLevel == "series")
      );
    },
    isVolViewButtonEnabled() {
      return this.resourceLevel == "study" || this.resourceLevel == "series";
    },
    hasOhifViewer() {
      return (
        this.uiOptions.EnableOpenInOhifViewer ||
        this.uiOptions.EnableOpenInOhifViewer3
      );
    },
    hasOhifViewerButton() {
      // Basic viewer
      if (
        !this.uiOptions.ViewersOrdering.includes("ohif") ||
        this.studiesSourceType != SourceType.LOCAL_ORTHANC
      ) {
        return false;
      }

      // disable if it only contains non images modalities:
      let modalities = this.modalitiesList;
      modalities = modalities.filter(
        (x) => !["SM", "ECG", "SR", "SEG"].includes(x),
      ); // since Set.difference is not supported on Firefox as of March 2024

      if (
        modalities.length == 0 &&
        this.resourceLevel != "bulk"
      ) // we can not check the modalities list for bulk mode
      {
        return false;
      }

      if (this.uiOptions.EnableOpenInOhifViewer3) {
        return (
          this.hasOhifViewer &&
          (this.resourceLevel == "study" ||
            (this.resourceLevel == "bulk" &&
              this.ohifDataSource == "dicom-web"))
        );
      } else {
        return this.hasOhifViewer && this.resourceLevel == "study";
      }
    },
    hasOhifViewerButtonVr() {
      if (
        !this.uiOptions.ViewersOrdering.includes("ohif-vr") ||
        this.studiesSourceType != SourceType.LOCAL_ORTHANC
      ) {
        return false;
      }

      // only for CT, PT and MR
      if (
        !(
          this.modalitiesList.includes("CT") ||
          this.modalitiesList.includes("PT") ||
          this.modalitiesList.includes("MR")
        ) &&
        this.resourceLevel != "bulk"
      ) // we can not check the modalities list for bulk mode)
      {
        return false;
      }

      if (this.uiOptions.EnableOpenInOhifViewer3) {
        return (
          this.hasOhifViewer &&
          (this.resourceLevel == "study" ||
            (this.resourceLevel == "bulk" &&
              this.ohifDataSource == "dicom-web"))
        );
      } else {
        return false;
      }
    },
    hasOhifViewerButtonTmtv() {
      if (
        !this.uiOptions.ViewersOrdering.includes("ohif-tmtv") ||
        this.studiesSourceType != SourceType.LOCAL_ORTHANC
      ) {
        return false;
      }

      // from isValidMode() in OHIF code
      if (
        !(
          this.modalitiesList.includes("CT") &&
          this.modalitiesList.includes("PT") &&
          !this.modalitiesList.includes("SM")
        ) &&
        this.resourceLevel != "bulk"
      ) // we can not check the modalities list for bulk mode)
      {
        return false;
      }

      if (this.uiOptions.EnableOpenInOhifViewer3) {
        return (
          this.hasOhifViewer &&
          (this.resourceLevel == "study" ||
            (this.resourceLevel == "bulk" &&
              this.ohifDataSource == "dicom-web"))
        );
      } else {
        return false;
      }
    },
    hasOhifViewerButtonSeg() {
      if (
        !this.uiOptions.ViewersOrdering.includes("ohif-seg") ||
        this.studiesSourceType != SourceType.LOCAL_ORTHANC
      ) {
        return false;
      }

      if (this.uiOptions.EnableOpenInOhifViewer3 && this.hasOhifViewer) {
        if (
          this.resourceLevel == "bulk" &&
          this.ohifDataSource == "dicom-web"
        ) {
          return true; // unable to check the list of modalities in this case -> allow it
        } else if (this.resourceLevel == "study") {
          // // from isValidMode() in OHIF code
          // Don't show the mode if the selected studies have only one modality that is not supported by the mode
          if (
            this.modalitiesList.length == 1 &&
            ["SM", "ECG", "OT", "DOC"].includes(this.modalitiesList[0])
          ) {
            return false;
          } else {
            return true;
          }
        }
      }

      return false;
    },
    hasOhifViewerButtonMicroscopy() {
      if (
        !this.uiOptions.ViewersOrdering.includes("ohif-micro") ||
        this.studiesSourceType != SourceType.LOCAL_ORTHANC
      ) {
        return false;
      }

      // Must have at least one SM series
      if (!this.modalitiesList.includes("SM") && this.resourceLevel != "bulk") {
        // we can not check the modalities list for bulk mode)
        return false;
      }

      if (this.uiOptions.EnableOpenInOhifViewer3) {
        return (
          this.hasOhifViewer &&
          (this.resourceLevel == "study" ||
            (this.resourceLevel == "bulk" &&
              this.ohifDataSource == "dicom-web"))
        );
      } else {
        return false;
      }
    },
    ohifViewerUrl() {
      return this.getOhifViewerUrl("basic");
    },
    ohifViewerUrlVr() {
      return this.getOhifViewerUrl("vr");
    },
    ohifViewerUrlTmtv() {
      return this.getOhifViewerUrl("tmtv");
    },
    ohifViewerUrlSeg() {
      return this.getOhifViewerUrl("seg");
    },
    ohifViewerUrlMicro() {
      return this.getOhifViewerUrl("microscopy");
    },
    isOhifButtonBasicViewerEnabled() {
      if (this.uiOptions.EnableOpenInOhifViewer3) {
        // OHIF V3
        return (
          this.resourceLevel == "study" ||
          (this.resourceLevel == "bulk" && this.selectedStudiesIds.length > 0)
        );
      } else {
        // OHIF V2
        return this.resourceLevel == "study";
      }
    },
    isOhifButtonVrEnabled() {
      if (this.uiOptions.EnableOpenInOhifViewer3) {
        // OHIF V3
        return (
          this.resourceLevel == "study" ||
          (this.resourceLevel == "bulk" && this.selectedStudiesIds.length > 0)
        );
      } else {
        // OHIF V2
        return false;
      }
    },
    isOhifButtonTmtvEnabled() {
      if (this.uiOptions.EnableOpenInOhifViewer3) {
        // OHIF V3
        return (
          this.resourceLevel == "study" ||
          (this.resourceLevel == "bulk" && this.selectedStudiesIds.length > 0)
        );
      } else {
        // OHIF V2
        return false;
      }
    },
    isOhifButtonMicroscopyEnabled() {
      if (this.uiOptions.EnableOpenInOhifViewer3) {
        // OHIF V3

        return (
          this.resourceLevel == "study" ||
          (this.resourceLevel == "bulk" && this.selectedStudiesIds.length > 0)
        );
      } else {
        // OHIF V2
        return false;
      }
    },
    isOhifButtonSegEnabled() {
      if (this.uiOptions.EnableOpenInOhifViewer3) {
        // OHIF V3
        return (
          this.resourceLevel == "study" ||
          (this.resourceLevel == "bulk" && this.selectedStudiesIds.length > 0)
        );
      } else {
        // OHIF V2
        return false;
      }
    },
    hasExportToNifti() {
      return this.isNiftiCompatible;
    },
    exportToNiftiUrl() {
      return api.getExportToNiftiUrl(
        this.resourceLevel,
        this.resourceOrthancId,
      );
    },
    hasStlViewer() {
      return (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        this.isPluginEnabled("stl")
      );
    },
    stlViewerUrl() {
      return api.getStlViewerUrl(this.resourceOrthancId);
    },
    hasStlViewerButton() {
      return (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        this.hasStlViewer &&
        this.resourceLevel == "instance" &&
        this.isStl
      );
    },
    isStlViewerButtonEnabled() {
      return this.resourceLevel == "instance";
    },
    hasWeasisViewer() {
      return (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        this.uiOptions.EnableOpenInWeasisViewer &&
        this.isPluginEnabled("dicom-web")
      );
    },
    weasisViewerUrl() {
      if (this.resourceLevel == "bulk") {
        const selectedStudiesDicomIds = this.selectedStudies.map(
          (s) => s["MainDicomTags"]["StudyInstanceUID"],
        );
        const url = api.getWeasisViewerUrlForBulkStudies(
          selectedStudiesDicomIds,
        );
        return url;
      } else {
        return api.getWeasisViewerUrl(this.resourceDicomUid);
      }
    },
    hasWeasisViewerButton() {
      return (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        this.hasWeasisViewer &&
        (this.resourceLevel == "study" || this.resourceLevel == "bulk")
      );
    },
    isWeasisViewerButtonEnabled() {
      return (
        this.resourceLevel == "study" ||
        (this.resourceLevel == "bulk" && this.selectedStudiesIds.length > 0)
      );
    },
    weasisViewerIcon() {
      return this.getViewerIcon("weasis");
    },
    hasMedDreamViewer() {
      return (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        this.uiOptions.EnableOpenInMedDreamViewer
      );
    },
    hasMedDreamViewerButton() {
      return (
        this.hasMedDreamViewer &&
        (this.resourceLevel == "study" || this.resourceLevel == "bulk")
      );
    },
    isMedDreamViewerButtonEnabled() {
      return (
        this.resourceLevel == "study" ||
        (this.resourceLevel == "bulk" && this.selectedStudiesIds.length > 0)
      );
    },
    medDreamViewerUrl() {
      return (
        this.uiOptions.MedDreamViewerPublicRoot +
        "?study=" +
        this.resourceDicomUid
      );
    },
    isStl() {
      if (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        this.resourceLevel == "instance"
      ) {
        return (
          this.instanceHeaders["0002,0002"]["Value"] ==
          "1.2.840.10008.5.1.4.1.1.104.3"
        );
      } else {
        return false;
      }
    },
    isPdfPreview() {
      if (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        this.resourceLevel == "instance"
      ) {
        return (
          this.instanceHeaders["0002,0002"]["Value"] ==
          "1.2.840.10008.5.1.4.1.1.104.1"
        );
      } else {
        return false;
      }
    },
    hasInstancePreviewButton() {
      return (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        this.resourceLevel == "instance" &&
        !this.isStl
      );
    },
    instancePreviewUrl() {
      if (this.isPdfPreview) {
        return api.getInstancePdfUrl(this.resourceOrthancId);
      } else {
        return api.getInstancePreviewUrl(this.resourceOrthancId);
      }
    },
    hasInstanceDownloadButton() {
      return (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        this.uiOptions.EnableDownloadDicomFile &&
        this.resourceLevel == "instance"
      );
    },
    instanceDownloadUrl() {
      let filename = null;
      if (this.resourceLevel == "instance") {
        console.log(this.instanceTags);
        filename = resourceHelpers.replaceResourceTagsInStringPlainText(
          this.uiOptions.DownloadInstanceFileNameTemplate,
          this.patientMainDicomTags,
          this.studyMainDicomTags,
          this.seriesMainDicomTags,
          this.instanceTags,
          this.resourceOrthancId,
          this.resourceLevel,
        );
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
      if (this.resourceLevel == "study" || this.resourceLevel == "series") {
        if (this.resourceLevel == "study") {
          filename = resourceHelpers.replaceResourceTagsInStringPlainText(
            this.uiOptions.DownloadStudyFileNameTemplate,
            this.patientMainDicomTags,
            this.studyMainDicomTags,
            this.seriesMainDicomTags,
            this.instanceTags,
            this.resourceOrthancId,
            this.resourceLevel,
          );
        } else if (this.resourceLevel == "series") {
          filename = resourceHelpers.replaceResourceTagsInStringPlainText(
            this.uiOptions.DownloadSeriesFileNameTemplate,
            this.patientMainDicomTags,
            this.studyMainDicomTags,
            this.seriesMainDicomTags,
            this.instanceTags,
            this.resourceOrthancId,
            this.resourceLevel,
          );
        }
      }

      return api.getDownloadZipUrl(
        this.resourceLevel,
        this.resourceOrthancId,
        filename,
      );
    },
    downloadDicomDirUrl() {
      let filename = null;
      if (this.resourceLevel == "study" || this.resourceLevel == "series") {
        if (this.resourceLevel == "study") {
          filename = resourceHelpers.replaceResourceTagsInStringPlainText(
            this.uiOptions.DownloadStudyFileNameTemplate,
            this.patientMainDicomTags,
            this.studyMainDicomTags,
            this.seriesMainDicomTags,
            this.instanceTags,
            this.resourceOrthancId,
            this.resourceLevel,
          );
        } else if (this.resourceLevel == "series") {
          filename = resourceHelpers.replaceResourceTagsInStringPlainText(
            this.uiOptions.DownloadSeriesFileNameTemplate,
            this.patientMainDicomTags,
            this.studyMainDicomTags,
            this.seriesMainDicomTags,
            this.instanceTags,
            this.resourceOrthancId,
            this.resourceLevel,
          );
        }
      }
      return api.getDownloadDicomDirUrl(
        this.resourceLevel,
        this.resourceOrthancId,
        filename,
      );
    },
    resourceTitle() {
      return resourceHelpers.getResourceTitle(
        this.resourceLevel,
        this.patientMainDicomTags,
        this.studyMainDicomTags,
        this.seriesMainDicomTags,
        this.instanceTags,
      );
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
    stlViewerIcon() {
      return this.getViewerIcon("stl");
    },
    deleteResourceTitle() {
      const texts = {
        study: "delete_study_title",
        series: "delete_series_title",
        instance: "delete_instance_title",
        bulk: "delete_studies_title",
      };
      return texts[this.resourceLevel];
    },
    deleteResourceBody() {
      const texts = {
        study: "delete_study_body",
        series: "delete_series_body",
        instance: "delete_instance_body",
        bulk: "delete_studies_body",
      };
      return texts[this.resourceLevel];
    },
    hasDownloadZipButton() {
      return (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        this.uiOptions.EnableDownloadZip &&
        this.resourceLevel != "instance" &&
        this.resourceLevel != "bulk"
      );
    },
    hasBulkDownloadZipButton() {
      if (
        this.system.ApiVersion < 22
      ) // the /tools/create-archive GET route has been introduced in 1.12.2
      {
        return false;
      }
      return (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        this.uiOptions.EnableDownloadZip &&
        this.resourceLevel == "bulk"
      );
    },
    isBulkDownloadZipEnabled() {
      return this.selectedStudiesIds.length > 0;
    },
    hasDownloadDicomDirButton() {
      return (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        this.uiOptions.EnableDownloadDicomDir &&
        this.resourceLevel != "instance" &&
        this.resourceLevel != "bulk"
      );
    },
    hasBulkDownloadDicomDirButton() {
      if (
        this.system.ApiVersion < 22
      ) // the /tools/create-media GET route has been introduced in 1.12.2
      {
        return false;
      }
      return (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        this.uiOptions.EnableDownloadDicomDir &&
        this.resourceLevel == "bulk"
      );
    },
    isBulkDownloadDicomDirEnabled() {
      return this.selectedStudiesIds.length > 0;
    },
    hasRetrieveButton() {
      return (
        this.studiesSourceType == SourceType.REMOTE_DICOM ||
        this.studiesSourceType == SourceType.REMOTE_DICOM_WEB
      );
    },
    isRetrieveButtonEnabled() {
      return (
        this.resourceLevel != "bulk" ||
        (this.resourceLevel == "bulk" && this.selectedStudiesIds.length > 0)
      );
    },
    hasAnonymizationButton() {
      return (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        this.resourceLevel != "bulk"
      );
    },
    isAnonymizationEnabled() {
      if (this.resourceLevel == "study" || this.resourceLevel == "series") {
        return this.uiOptions.EnableAnonymization;
      } else {
        return false;
      }
    },
    hasModificationButton() {
      return (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        this.resourceLevel != "bulk"
      );
    },
    isModificationEnabled() {
      if (this.resourceLevel == "study" || this.resourceLevel == "series") {
        return this.uiOptions.EnableModification;
      } else {
        return false;
      }
    },
    hasApiViewButton() {
      return (
        this.resourceLevel != "bulk" &&
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        this.uiOptions.EnableApiViewMenu
      );
    },
    resourcesOrthancId() {
      if (this.resourceLevel == "bulk") {
        return this.selectedStudiesIds;
      } else {
        return [this.resourceOrthancId];
      }
    },
    resourcesForTransfer() {
      if (this.resourceLevel == "bulk") {
        return this.selectedStudiesIds.map((id) => ({
          Level: "Study",
          ID: id,
        }));
      } else {
        return [
          {
            Level: this.capitalizeFirstLetter(this.resourceLevel),
            ID: this.resourceOrthancId,
          },
        ];
      }
    },
    computedResourceLevel() {
      if (this.resourceLevel == "bulk") {
        return "study";
      } else {
        return this.resourceLevel;
      }
    },
    resourceLevelForCustomButtons() {
      if (this.resourceLevel == "bulk") {
        return "bulk-studies";
      } else {
        return this.resourceLevel;
      }
    },
    hasAccessToWorklists() {
      return (
        this.isPluginEnabled("orthanc-worklists") &&
        this.uiOptions.EnableWorklists &&
        this.resourceLevel == "study"
      );
    },
    hasCustomButtons() {
      return (
        this.uiOptions.CustomButtons &&
        this.uiOptions.CustomButtons[this.resourceLevelForCustomButtons] &&
        this.uiOptions.CustomButtons[this.resourceLevelForCustomButtons]
          .length > 0
      );
    },
    CustomButtons() {
      let customButtons = [];
      for (const customButton of this.uiOptions.CustomButtons[
        this.resourceLevelForCustomButtons
      ]) {
        let cloneCustomButton = Object.assign({}, customButton);
        customButtons.push(cloneCustomButton);
      }

      return customButtons;
    },
    isCustomButtonEnabled() {
      if (this.resourceLevel == "bulk") {
        // console.log("isCustomButtonEnabled", this.selectedStudiesIds.length > 0);
        return this.selectedStudiesIds.length > 0;
      } else {
        return true;
      }
    },
    buttonClasses() {
      if (this.smallIcons) {
        return "btn-icon-small";
      } else {
        return "btn-icon";
      }
    },
  },
  components: {
    Modal,
    ShareModal,
    ModifyModal,
    TokenLinkButton,
    BulkLabelsModal,
    AddSeriesModal,
    CreateWorklistModal,
  },
};
</script>

<template>
  <div class="custom-button-group">
    <span v-for="viewer in uiOptions.ViewersOrdering" :key="viewer">
      <TokenLinkButton
        v-if="viewer == 'meddream' && hasMedDreamViewerButton"
        :disabled="!isMedDreamViewerButtonEnabled"
        :iconClass="medDreamViewerIcon"
        :level="computedResourceLevel"
        :linkUrl="medDreamViewerUrl"
        :resourcesOrthancId="resourcesOrthancId"
        :title="$t('view_in_meddream')"
        :tokenType="'meddream-instant-link'"
        :opensInNewTab="true"
        :smallIcons="smallIcons"
      >
      </TokenLinkButton>

      <TokenLinkButton
        v-if="
          hasOsimisViewer &&
          viewer == 'osimis-web-viewer' &&
          (this.resourceLevel == 'study' || this.resourceLevel == 'series')
        "
        :iconClass="osimisViewerIcon"
        :level="this.resourceLevel"
        :linkUrl="osimisViewerUrl"
        :resourcesOrthancId="resourcesOrthancId"
        :title="$t('view_in_osimis')"
        :tokenType="'viewer-instant-link'"
        :opensInNewTab="true"
        :smallIcons="smallIcons"
      >
      </TokenLinkButton>

      <TokenLinkButton
        v-if="viewer == 'stone-webviewer' && hasStoneViewerButton"
        :disabled="!isStoneViewerButtonEnabled"
        :iconClass="stoneViewerIcon"
        :level="computedResourceLevel"
        :linkUrl="stoneViewerUrl"
        :resourcesOrthancId="resourcesOrthancId"
        :title="$t('view_in_stone')"
        :tokenType="'viewer-instant-link'"
        :opensInNewTab="true"
        :smallIcons="smallIcons"
      >
      </TokenLinkButton>

      <TokenLinkButton
        v-if="viewer == 'volview' && hasVolViewButton"
        :disabled="!isVolViewButtonEnabled"
        :iconClass="volViewIcon"
        :level="computedResourceLevel"
        :linkUrl="volViewUrl"
        :resourcesOrthancId="resourcesOrthancId"
        :title="$t('view_in_volview')"
        :tokenType="'viewer-instant-link'"
        :opensInNewTab="true"
        :smallIcons="smallIcons"
      >
      </TokenLinkButton>

      <TokenLinkButton
        v-if="viewer == 'ohif' && hasOhifViewerButton"
        :disabled="!isOhifButtonBasicViewerEnabled"
        :iconClass="ohifViewerIcon"
        :level="computedResourceLevel"
        :linkUrl="ohifViewerUrl"
        :resourcesOrthancId="resourcesOrthancId"
        :title="$t('view_in_ohif')"
        :tokenType="'viewer-instant-link'"
        :opensInNewTab="true"
        :smallIcons="smallIcons"
      >
      </TokenLinkButton>

      <TokenLinkButton
        v-if="viewer == 'ohif-vr' && hasOhifViewerButtonVr"
        :disabled="!isOhifButtonVrEnabled"
        :iconClass="ohifViewerIconVr"
        :level="computedResourceLevel"
        :linkUrl="ohifViewerUrlVr"
        :resourcesOrthancId="resourcesOrthancId"
        :title="$t('view_in_ohif_vr')"
        :tokenType="'viewer-instant-link'"
        :opensInNewTab="true"
        :smallIcons="smallIcons"
      >
      </TokenLinkButton>

      <TokenLinkButton
        v-if="viewer == 'ohif-tmtv' && hasOhifViewerButtonTmtv"
        :disabled="!isOhifButtonTmtvEnabled"
        :iconClass="ohifViewerIconTmtv"
        :level="computedResourceLevel"
        :linkUrl="ohifViewerUrlTmtv"
        :resourcesOrthancId="resourcesOrthancId"
        :title="$t('view_in_ohif_tmtv')"
        :tokenType="'viewer-instant-link'"
        :opensInNewTab="true"
        :smallIcons="smallIcons"
      >
      </TokenLinkButton>

      <TokenLinkButton
        v-if="viewer == 'ohif-seg' && hasOhifViewerButtonSeg"
        :disabled="!isOhifButtonSegEnabled"
        :iconClass="ohifViewerIconSeg"
        :level="computedResourceLevel"
        :linkUrl="ohifViewerUrlSeg"
        :resourcesOrthancId="resourcesOrthancId"
        :title="$t('view_in_ohif_seg')"
        :tokenType="'viewer-instant-link'"
        :opensInNewTab="true"
        :smallIcons="smallIcons"
      >
      </TokenLinkButton>

      <TokenLinkButton
        v-if="viewer == 'ohif-micro' && hasOhifViewerButtonMicroscopy"
        :disabled="!isOhifButtonMicroscopyEnabled"
        :iconClass="ohifViewerIconMicro"
        :level="computedResourceLevel"
        :linkUrl="ohifViewerUrlMicro"
        :resourcesOrthancId="resourcesOrthancId"
        :title="$t('view_in_ohif_microscopy')"
        :tokenType="'viewer-instant-link'"
        :opensInNewTab="true"
        :smallIcons="smallIcons"
      >
      </TokenLinkButton>
      <TokenLinkButton
        v-if="viewer == 'stl' && hasStlViewerButton"
        :disabled="!isStlViewerButtonEnabled"
        :iconClass="stlViewerIcon"
        :level="computedResourceLevel"
        :linkUrl="stlViewerUrl"
        :resourcesOrthancId="resourcesOrthancId"
        :title="$t('view_in_stl_viewer')"
        :tokenType="'viewer-instant-link'"
        :opensInNewTab="true"
        :smallIcons="smallIcons"
      >
      </TokenLinkButton>
      <TokenLinkButton
        v-if="viewer == 'weasis' && hasWeasisViewerButton"
        :disabled="!isWeasisViewerButtonEnabled"
        :iconClass="weasisViewerIcon"
        :level="computedResourceLevel"
        :linkUrl="weasisViewerUrl"
        :resourcesOrthancId="resourcesOrthancId"
        :title="$t('view_in_weasis')"
        :tokenType="'viewer-instant-link'"
        :opensInNewTab="true"
        :smallIcons="smallIcons"
      >
      </TokenLinkButton>
    </span>
    <TokenLinkButton
      v-if="hasInstancePreviewButton"
      :iconClass="'bi bi-binoculars'"
      :level="this.resourceLevel"
      :linkUrl="instancePreviewUrl"
      :resourcesOrthancId="[resourceOrthancId]"
      :title="$t('preview')"
      :tokenType="'download-instant-link'"
      :opensInNewTab="true"
      :smallIcons="smallIcons"
    >
    </TokenLinkButton>
    <TokenLinkButton
      v-if="hasWsiButton"
      :hidden="!isWsiButtonEnabled"
      :iconClass="wsiViewerIcon"
      :level="this.resourceLevel"
      :linkUrl="wsiViewerUrl"
      :resourcesOrthancId="resourcesOrthancId"
      :title="$t('view_in_wsi_viewer')"
      :tokenType="'viewer-instant-link'"
      :opensInNewTab="true"
      :smallIcons="smallIcons"
    >
    </TokenLinkButton>
  </div>
  <div class="custom-button-group">
    <TokenLinkButton
      v-if="hasDownloadZipButton"
      :iconClass="'bi bi-download'"
      :level="this.resourceLevel"
      :linkUrl="downloadZipUrl"
      :resourcesOrthancId="resourcesOrthancId"
      :title="$t('download_zip')"
      :tokenType="'download-instant-link'"
      :smallIcons="smallIcons"
    >
    </TokenLinkButton>
    <TokenLinkButton
      v-if="hasBulkDownloadZipButton"
      :iconClass="'bi bi-download'"
      :level="'bulk-study'"
      :linkUrl="downloadBulkZipUrl"
      :resourcesOrthancId="resourcesOrthancId"
      :title="$t('download_zip')"
      :tokenType="'download-instant-link'"
      :disabled="!isBulkDownloadZipEnabled"
      :smallIcons="smallIcons"
    >
    </TokenLinkButton>
    <TokenLinkButton
      v-if="hasDownloadDicomDirButton"
      :iconClass="'bi bi-box-arrow-down'"
      :level="this.resourceLevel"
      :linkUrl="downloadDicomDirUrl"
      :resourcesOrthancId="[resourceOrthancId]"
      :title="$t('download_dicomdir')"
      :tokenType="'download-instant-link'"
      :smallIcons="smallIcons"
    >
    </TokenLinkButton>
    <TokenLinkButton
      v-if="hasBulkDownloadDicomDirButton"
      :iconClass="'bi bi-box-arrow-down'"
      :level="'bulk-study'"
      :linkUrl="downloadBulkDicomDirUrl"
      :resourcesOrthancId="resourcesOrthancId"
      :title="$t('download_dicomdir')"
      :tokenType="'download-instant-link'"
      :disabled="!isBulkDownloadDicomDirEnabled"
      :smallIcons="smallIcons"
    >
    </TokenLinkButton>
    <TokenLinkButton
      v-if="hasInstanceDownloadButton"
      :iconClass="'bi bi-download'"
      :level="this.resourceLevel"
      :linkUrl="instanceDownloadUrl"
      :resourcesOrthancId="[resourceOrthancId]"
      :title="$t('download_dicom_file')"
      :tokenType="'download-instant-link'"
      :smallIcons="smallIcons"
    >
    </TokenLinkButton>
    <TokenLinkButton
      v-if="hasExportToNifti"
      :iconClass="'fa-solid fa-brain'"
      :level="this.resourceLevel"
      :linkUrl="exportToNiftiUrl"
      :resourcesOrthancId="[resourceOrthancId]"
      :title="$t('export_to_nifti')"
      :tokenType="'download-instant-link'"
      :opensInNewTab="true"
      :smallIcons="smallIcons"
    >
    </TokenLinkButton>
  </div>
  <div class="custom-button-group">
    <button
      v-if="hasDeleteButton"
      class="btn btn-sm btn-secondary m-1"
      type="button"
      data-bs-toggle="modal"
      v-bind:data-bs-target="'#delete-modal-' + this.resourceOrthancId"
      :disabled="!isDeleteEnabled"
      :class="buttonClasses"
    >
      <i class="bi bi-trash" data-bs-toggle="tooltip" :title="$t('delete')"></i>
    </button>
    <Modal
      v-if="hasDeleteButton"
      :id="'delete-modal-' + this.resourceOrthancId"
      :headerText="$t(this.deleteResourceTitle) + ' ' + this.resourceTitle"
      :okText="$t('delete')"
      :cancelText="$t('cancel')"
      :bodyText="$t(this.deleteResourceBody)"
      @ok="deleteResource($event)"
    >
    </Modal>
  </div>
  <div v-if="hasShareButton" class="custom-button-group">
    <button
      class="btn btn-sm btn-secondary m-1"
      type="button"
      data-bs-toggle="modal"
      v-bind:data-bs-target="'#share-modal-' + this.resourceOrthancId"
      :disabled="!isShareEnabled"
      :class="buttonClasses"
    >
      <i
        class="bi bi-share"
        data-bs-toggle="tooltip"
        :title="$t('share.button_title')"
      ></i>
    </button>
    <ShareModal
      :id="'share-modal-' + this.resourceOrthancId"
      :orthancId="this.resourceOrthancId"
      :studyMainDicomTags="this.studyMainDicomTags"
      :patientMainDicomTags="this.patientMainDicomTags"
    >
    </ShareModal>
  </div>
  <div v-if="hasModificationButton" class="custom-button-group">
    <button
      v-if="isModificationEnabled"
      class="btn btn-sm btn-secondary m-1"
      type="button"
      data-bs-toggle="modal"
      v-bind:data-bs-target="'#modify-modal-' + this.resourceOrthancId"
      :class="buttonClasses"
    >
      <i
        class="bi bi-pencil"
        data-bs-toggle="tooltip"
        :title="$t('modify.modify_button_title')"
      ></i>
    </button>
    <ModifyModal
      v-if="isModificationEnabled"
      :id="'modify-modal-' + this.resourceOrthancId"
      :orthancId="this.resourceOrthancId"
      :resourceLevel="this.resourceLevel"
      :seriesMainDicomTags="this.seriesMainDicomTags"
      :studyMainDicomTags="this.studyMainDicomTags"
      :patientMainDicomTags="this.patientMainDicomTags"
      :isAnonymization="false"
    ></ModifyModal>
  </div>
  <div v-if="hasAnonymizationButton" class="custom-button-group">
    <button
      v-if="isAnonymizationEnabled"
      class="btn btn-sm btn-secondary m-1"
      type="button"
      data-bs-toggle="modal"
      v-bind:data-bs-target="'#anonymize-modal-' + this.resourceOrthancId"
      :class="buttonClasses"
    >
      <i
        class="bi bi-person-slash"
        data-bs-toggle="tooltip"
        :title="$t('modify.anonymize_button_title')"
      ></i>
    </button>
    <ModifyModal
      v-if="isAnonymizationEnabled"
      :id="'anonymize-modal-' + this.resourceOrthancId"
      :orthancId="this.resourceOrthancId"
      :resourceLevel="this.resourceLevel"
      :seriesMainDicomTags="this.seriesMainDicomTags"
      :studyMainDicomTags="this.studyMainDicomTags"
      :patientMainDicomTags="this.patientMainDicomTags"
      :isAnonymization="true"
    ></ModifyModal>
  </div>
  <div v-if="hasAddSeriesButton" class="custom-button-group">
    <button
      class="btn btn-sm btn-secondary m-1"
      type="button"
      data-bs-toggle="modal"
      v-bind:data-bs-target="'#add-series-modal-' + this.resourceOrthancId"
      :class="buttonClasses"
    >
      <i
        class="bi bi-file-earmark-plus"
        data-bs-toggle="tooltip"
        :title="$t('add_series.button_title')"
      ></i>
    </button>
    <AddSeriesModal
      :id="'add-series-modal-' + this.resourceOrthancId"
      :orthancStudyId="this.resourceOrthancId"
      :studyMainDicomTags="this.studyMainDicomTags"
      :patientMainDicomTags="this.patientMainDicomTags"
    >
    </AddSeriesModal>
  </div>

  <div class="custom-button-group" v-if="hasApiViewButton">
    <div class="dropdown">
      <button
        class="dropdown btn btn-sm btn-secondary m-1 dropdown-toggle"
        type="button"
        id="apiDropdownMenuId"
        data-bs-toggle="dropdown"
        aria-expanded="false"
        :class="buttonClasses"
      >
        <span data-bs-toggle="tooltip" title="API">
          <i class="bi bi-code-slash"></i>
        </span>
      </button>
      <ul class="dropdown-menu" aria-labelledby="apiDropdownMenuId">
        <li>
          <button
            class="dropdown-item"
            href="#"
            @click="copyIdToClipboard"
            :class="buttonClasses"
          >
            {{ $t("copy_orthanc_id") }}
          </button>
        </li>
        <li>
          <TokenLinkButton
            :linkType="'dropdown-item'"
            :level="this.resourceLevel"
            :linkUrl="getApiUrl('')"
            :resourcesOrthancId="[resourceOrthancId]"
            :title="'/'"
            :tokenType="'download-instant-link'"
            :opensInNewTab="true"
            :smallIcons="smallIcons"
          >
          </TokenLinkButton>
        </li>
        <li v-if="this.resourceLevel == 'instance'">
          <TokenLinkButton
            :linkType="'dropdown-item'"
            :level="this.resourceLevel"
            :linkUrl="getApiUrl('/tags?simplify')"
            :resourcesOrthancId="[resourceOrthancId]"
            :title="'/tags?simplify'"
            :tokenType="'download-instant-link'"
            :opensInNewTab="true"
            :smallIcons="smallIcons"
          >
          </TokenLinkButton>
        </li>
        <li>
          <TokenLinkButton
            :linkType="'dropdown-item'"
            :level="this.resourceLevel"
            :linkUrl="getApiUrl('/metadata?expand')"
            :resourcesOrthancId="[resourceOrthancId]"
            :title="'/metadata?expand'"
            :tokenType="'download-instant-link'"
            :opensInNewTab="true"
            :smallIcons="smallIcons"
          >
          </TokenLinkButton>
        </li>
        <li>
          <TokenLinkButton
            :linkType="'dropdown-item'"
            :level="this.resourceLevel"
            :linkUrl="getApiUrl('/statistics')"
            :resourcesOrthancId="[resourceOrthancId]"
            :title="'/statistics'"
            :tokenType="'download-instant-link'"
            :opensInNewTab="true"
            :smallIcons="smallIcons"
          >
          </TokenLinkButton>
        </li>
        <li>
          <TokenLinkButton
            :linkType="'dropdown-item'"
            :level="this.resourceLevel"
            :linkUrl="getApiUrl('/attachments?expand')"
            :resourcesOrthancId="[resourceOrthancId]"
            :title="'/attachments?expand'"
            :tokenType="'download-instant-link'"
            :opensInNewTab="true"
            :smallIcons="smallIcons"
          >
          </TokenLinkButton>
        </li>
      </ul>
    </div>
  </div>
  <div class="custom-button-group" v-if="this.resourceLevel == 'bulk'">
    <!-- <button v-if="hasLabelsButton" class="btn btn-sm btn-secondary m-1" type="button"
                data-bs-toggle="modal" v-bind:data-bs-target="'#labels2-modal2-' + this.componentId" :disabled="!isLabelsEnabled">
                <i class="bi bi-tag" data-bs-toggle="tooltip" :title="$t('labels.edit_labels_button')" ></i>
            </button> -->
    <button
      v-if="hasLabelsButton"
      class="btn btn-sm btn-secondary m-1"
      type="button"
      :disabled="!isLabelsEnabled"
      @click="showBulkLabelModal()"
      :class="buttonClasses"
    >
      <i
        class="bi bi-tag"
        data-bs-toggle="tooltip"
        :title="$t('labels.edit_labels_button')"
      ></i>
    </button>
    <BulkLabelsModal
      v-if="uiOptions.EnableEditLabels && isBulkLabelModalVisible"
      :id="'labels2-modal2-' + this.componentId"
      ref="bulk-label-modal"
      :resourceLevel="this.resourceLevel"
      :resourcesOrthancId="selectedStudiesIds"
      @bulkModalClosed="onBulkModalClosed"
    >
    </BulkLabelsModal>
  </div>
  <div class="custom-button-group">
    <div class="dropdown">
      <button
        v-if="hasSendTo"
        class="dropdown btn btn-sm btn-secondary m-1 dropdown-toggle"
        type="button"
        id="sendToDropdownMenuId"
        data-bs-toggle="dropdown"
        aria-expanded="false"
        :disabled="!isSendToEnabled"
        :class="buttonClasses"
      >
        <span data-bs-toggle="tooltip" :title="$t('send_to.button_title')">
          <i class="bi bi-send"></i>
        </span>
      </button>
      <ul
        class="dropdown-menu"
        aria-labelledby="sendToDropdownMenuId"
        v-if="hasSendTo"
      >
        <li v-if="hasSendToPeers" class="dropdown-submenu">
          <a class="dropdown-item" @click="toggleSubMenu" href="#">
            {{ $t("send_to.orthanc_peer") }}
            <i class="bi bi-caret-down"></i>
          </a>
          <ul class="dropdown-menu">
            <li v-for="peer in orthancPeers" :key="peer">
              <a class="dropdown-item" @click="sendToOrthancPeer(peer)">{{
                peer
              }}</a>
            </li>
          </ul>
        </li>
        <li v-if="hasSendToPeersWithTransfer" class="dropdown-submenu">
          <a class="dropdown-item" @click="toggleSubMenu" href="#">
            {{ $t("send_to.transfers") }}
            <i class="bi bi-caret-down"></i>
          </a>
          <ul class="dropdown-menu">
            <li v-for="peer in orthancPeers" :key="peer">
              <a
                class="dropdown-item"
                @click="sendToOrthancPeerWithTransfers(peer)"
                >{{ peer }}</a
              >
            </li>
          </ul>
        </li>
        <li v-if="hasSendToDicomWeb" class="dropdown-submenu">
          <a class="dropdown-item" @click="toggleSubMenu" href="#">
            {{ $t("send_to.dicom_web") }}
            <i class="bi bi-caret-down"></i>
          </a>
          <ul class="dropdown-menu">
            <li v-for="dwServer in targetDicomWebServers" :key="dwServer">
              <a
                class="dropdown-item"
                @click="sendToDicomWebServer(dwServer)"
                >{{ dwServer }}</a
              >
            </li>
          </ul>
        </li>
        <li v-if="hasSendToDicomModalities" class="dropdown-submenu">
          <a class="dropdown-item" @click="toggleSubMenu" href="#">
            {{ $t("send_to.dicom") }}
            <i class="bi bi-caret-down"></i>
          </a>
          <ul class="dropdown-menu">
            <li
              v-for="modality in Object.keys(targetDicomModalities)"
              :key="modality"
            >
              <a class="dropdown-item" @click="sendToDicomModality(modality)">{{
                modality
              }}</a>
            </li>
          </ul>
        </li>
      </ul>
    </div>
  </div>
  <div v-if="hasRetrieveButton" class="custom-button-group">
    <button
      class="btn btn-sm btn-secondary m-1"
      type="button"
      :disabled="!isRetrieveButtonEnabled"
      @click="retrieve"
      :class="buttonClasses"
    >
      <i
        class="bi bi-box-arrow-in-down"
        data-bs-toggle="tooltip"
        :title="$t('retrieve')"
      ></i>
    </button>
  </div>
  <div v-if="hasCustomButtons" class="custom-button-group">
    <div v-for="customButton in CustomButtons" :key="customButton.Id" class>
      <button
        class="btn btn-sm m-1 btn-secondary custom-button"
        type="button"
        :disabled="!isCustomButtonEnabled"
        data-bs-toggle="tooltip"
        :title="customButton.Tooltip"
        :target="customButton.Target"
        @click="customButtonClicked(customButton)"
        :class="buttonClasses"
      >
        <i :class="customButton.Icon"></i>
      </button>
    </div>
  </div>
  <div v-if="hasAccessToWorklists" class="custom-button-group">
    <button
      class="btn btn-sm btn-secondary m-1"
      type="button"
      data-bs-toggle="modal"
      v-bind:data-bs-target="
        '#create-modal-worklists-' + this.resourceOrthancId
      "
      :class="buttonClasses"
    >
      <i
        class="bi bi-calendar-plus"
        data-bs-toggle="tooltip"
        :title="$t('worklists.create_worklist_for_this_patient')"
      ></i>
    </button>
    <CreateWorklistModal
      :id="'create-modal-worklists-' + this.resourceOrthancId"
      :orthancStudyId="this.resourceOrthancId"
      :reloadWindowAfterCreation="false"
    />
  </div>
</template>

<style>
/* =============================================================================
   RESOURCEBUTTONGROUP.VUE - MODERN STYLES (D-MIS Design System)
   
   Современные стили для кнопок управления ресурсами
   ============================================================================= */

/* =============================================================================
   CONTAINER LAYOUT
   ============================================================================= */

/* Основной контейнер для кнопок */
.resource-button-group,
.study-button-group {
  display: flex;
  flex-direction: column;
  gap: 12px;
  align-items: flex-end;
  padding: 0;
  min-width: 60px;
}

/* Horizontal layout */
.resource-button-group.horizontal,
.study-button-group.horizontal {
  flex-direction: row;
  flex-wrap: wrap;
  justify-content: flex-end;
  align-items: center;
}

/* =============================================================================
   BASE BUTTON RESET & COMMON STYLES
   ============================================================================= */

.resource-button-group button,
.resource-button-group a,
.resource-button-group .btn {
  all: unset;
  box-sizing: border-box;
  cursor: pointer;
  font-family:
    "Segoe UI",
    system-ui,
    -apple-system,
    sans-serif;
  transition: all 0.2s ease;
  user-select: none;
}

/* =============================================================================
   ICON-ONLY BUTTONS (основной стиль для вертикального ряда)
   ============================================================================= */

.resource-button-group button,
.resource-button-group a,
.resource-button-group .btn {
  width: 48px;
  height: 48px;
  min-width: 48px;
  padding: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  background: linear-gradient(135deg, #64748b 0%, #475569 100%);
  border: none;
  border-radius: 10px;
  color: white;
  font-size: 20px;
  box-shadow: 0 2px 6px rgba(100, 116, 139, 0.25);
  position: relative;
}

.resource-button-group button:hover,
.resource-button-group a:hover,
.resource-button-group .btn:hover {
  background: linear-gradient(135deg, #475569 0%, #334155 100%);
  box-shadow: 0 4px 12px rgba(100, 116, 139, 0.35);
  transform: scale(1.08);
}

.resource-button-group button:active,
.resource-button-group a:active,
.resource-button-group .btn:active {
  transform: scale(1.03);
  box-shadow: 0 2px 4px rgba(100, 116, 139, 0.2);
}

/* Icon inside button */
.resource-button-group button i,
.resource-button-group a i,
.resource-button-group .btn i {
  font-size: 20px;
  color: white;
  line-height: 1;
  transition: none;
}

/* =============================================================================
   COLOR VARIANTS
   ============================================================================= */

/* Primary (Blue) - View/Open actions */
.resource-button-group .btn-primary,
.resource-button-group button[data-action="view"],
.resource-button-group a[href*="viewer"] {
  background: linear-gradient(135deg, #3b82f6 0%, #2563eb 100%);
}

.resource-button-group .btn-primary:hover,
.resource-button-group button[data-action="view"]:hover,
.resource-button-group a[href*="viewer"]:hover {
  background: linear-gradient(135deg, #2563eb 0%, #1d4ed8 100%);
  box-shadow: 0 4px 12px rgba(59, 130, 246, 0.35);
}

/* Download (Purple) */
.resource-button-group .btn-download,
.resource-button-group button[data-action="download"] {
  background: linear-gradient(135deg, #8b5cf6 0%, #7c3aed 100%);
}

.resource-button-group .btn-download:hover,
.resource-button-group button[data-action="download"]:hover {
  background: linear-gradient(135deg, #7c3aed 0%, #6d28d9 100%);
  box-shadow: 0 4px 12px rgba(139, 92, 246, 0.35);
}

/* Send (Cyan) */
.resource-button-group .btn-send,
.resource-button-group button[data-action="send"] {
  background: linear-gradient(135deg, #06b6d4 0%, #0891b2 100%);
}

.resource-button-group .btn-send:hover,
.resource-button-group button[data-action="send"]:hover {
  background: linear-gradient(135deg, #0891b2 0%, #0e7490 100%);
  box-shadow: 0 4px 12px rgba(6, 182, 212, 0.35);
}

/* Edit (Orange) */
.resource-button-group .btn-edit,
.resource-button-group button[data-action="edit"] {
  background: linear-gradient(135deg, #f59e0b 0%, #d97706 100%);
}

.resource-button-group .btn-edit:hover,
.resource-button-group button[data-action="edit"]:hover {
  background: linear-gradient(135deg, #d97706 0%, #b45309 100%);
  box-shadow: 0 4px 12px rgba(245, 158, 11, 0.35);
}

/* Delete (Red) */
.resource-button-group .btn-danger,
.resource-button-group .btn-delete,
.resource-button-group button[data-action="delete"] {
  background: linear-gradient(135deg, #ef4444 0%, #dc2626 100%);
}

.resource-button-group .btn-danger:hover,
.resource-button-group .btn-delete:hover,
.resource-button-group button[data-action="delete"]:hover {
  background: linear-gradient(135deg, #dc2626 0%, #b91c1c 100%);
  box-shadow: 0 4px 12px rgba(239, 68, 68, 0.35);
}

/* Success (Green) */
.resource-button-group .btn-success,
.resource-button-group button[data-action="approve"] {
  background: linear-gradient(135deg, #10b981 0%, #059669 100%);
}

.resource-button-group .btn-success:hover,
.resource-button-group button[data-action="approve"]:hover {
  background: linear-gradient(135deg, #059669 0%, #047857 100%);
  box-shadow: 0 4px 12px rgba(16, 185, 129, 0.35);
}

/* =============================================================================
   BUTTON STATES
   ============================================================================= */

/* Disabled */
.resource-button-group button:disabled,
.resource-button-group .btn:disabled {
  background: linear-gradient(135deg, #cbd5e1 0%, #94a3b8 100%);
  color: #64748b;
  cursor: not-allowed;
  box-shadow: none;
  opacity: 0.5;
}

.resource-button-group button:disabled:hover,
.resource-button-group .btn:disabled:hover {
  transform: none;
  box-shadow: none;
}

/* Loading */
.resource-button-group button.loading,
.resource-button-group .btn.loading {
  color: transparent;
  pointer-events: none;
}

.resource-button-group button.loading::after,
.resource-button-group .btn.loading::after {
  content: "";
  position: absolute;
  width: 20px;
  height: 20px;
  border: 2px solid rgba(255, 255, 255, 0.3);
  border-top-color: white;
  border-radius: 50%;
  animation: spin 0.6s linear infinite;
}

@keyframes spin {
  to {
    transform: rotate(360deg);
  }
}

/* Active/Selected */
.resource-button-group button.active,
.resource-button-group .btn.active {
  background: linear-gradient(135deg, #3b82f6 0%, #2563eb 100%);
  box-shadow: 0 4px 12px rgba(59, 130, 246, 0.4);
  transform: scale(1.05);
}

/* =============================================================================
   TOOLTIPS
   ============================================================================= */

.resource-button-group button[title],
.resource-button-group .btn[title] {
  position: relative;
}

/* Показывать tooltip при hover */
.resource-button-group button[title]:hover::before,
.resource-button-group .btn[title]:hover::before {
  content: attr(title);
  position: absolute;
  right: calc(100% + 8px);
  top: 50%;
  transform: translateY(-50%);
  background: rgba(0, 0, 0, 0.9);
  color: white;
  padding: 6px 12px;
  border-radius: 6px;
  font-size: 12px;
  font-weight: 500;
  white-space: nowrap;
  pointer-events: none;
  z-index: 1000;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.3);
}

/* Стрелка tooltip */
.resource-button-group button[title]:hover::after,
.resource-button-group .btn[title]:hover::after {
  content: "";
  position: absolute;
  right: calc(100% + 2px);
  top: 50%;
  transform: translateY(-50%);
  border: 6px solid transparent;
  border-left-color: rgba(0, 0, 0, 0.9);
  pointer-events: none;
  z-index: 1000;
}

/* =============================================================================
   SMALL BUTTONS (для компактного отображения)
   ============================================================================= */

.resource-button-group.small button,
.resource-button-group.small .btn,
.resource-button-group .btn-sm {
  width: 40px;
  height: 40px;
  min-width: 40px;
  font-size: 16px;
  border-radius: 8px;
}

.resource-button-group.small button i,
.resource-button-group .btn-sm i {
  font-size: 16px;
}

/* =============================================================================
   LARGE BUTTONS
   ============================================================================= */

.resource-button-group.large button,
.resource-button-group.large .btn,
.resource-button-group .btn-lg {
  width: 56px;
  height: 56px;
  min-width: 56px;
  font-size: 24px;
  border-radius: 12px;
}

.resource-button-group.large button i,
.resource-button-group .btn-lg i {
  font-size: 24px;
}

/* =============================================================================
   DROPDOWN BUTTONS
   ============================================================================= */

.resource-button-group .dropdown-toggle {
  position: relative;
}

.resource-button-group .dropdown-toggle::after {
  content: "";
  position: absolute;
  right: 4px;
  bottom: 4px;
  width: 0;
  height: 0;
  border-left: 4px solid transparent;
  border-right: 4px solid transparent;
  border-top: 4px solid white;
}

/* =============================================================================
   RESPONSIVE DESIGN
   ============================================================================= */

@media (max-width: 768px) {
  .resource-button-group {
    flex-direction: row;
    justify-content: center;
    gap: 8px;
  }

  .resource-button-group button,
  .resource-button-group .btn {
    width: 44px;
    height: 44px;
    min-width: 44px;
    font-size: 18px;
  }
}

/* =============================================================================
   ACCESSIBILITY
   ============================================================================= */

.resource-button-group button:focus-visible,
.resource-button-group .btn:focus-visible {
  outline: 2px solid #3b82f6;
  outline-offset: 2px;
}

/* High contrast mode */
@media (prefers-contrast: high) {
  .resource-button-group button,
  .resource-button-group .btn {
    border: 2px solid rgba(255, 255, 255, 0.3);
  }
}

/* Reduced motion */
@media (prefers-reduced-motion: reduce) {
  .resource-button-group button,
  .resource-button-group .btn {
    transition: none;
    animation: none;
  }
}

/* =============================================================================
   PRINT STYLES
   ============================================================================= */

@media print {
  .resource-button-group {
    display: none !important;
  }
}
</style>
