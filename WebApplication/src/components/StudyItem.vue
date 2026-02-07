<script>
import SeriesList from "./SeriesList.vue";
import StudyDetails from "./StudyDetails.vue";
import { mapState } from "vuex";
import bootstrap from "bootstrap/dist/js/bootstrap.bundle.min.js";
import api from "../orthancApi";
import dateHelpers from "../helpers/date-helpers";
import SourceType from "../helpers/source-type";
import resourceHelpers from "../helpers/resource-helpers";
import TokenLinkButton from "./TokenLinkButton.vue";

export default {
  props: ["studyId"],
  emits: ["deletedStudy", "filter-modality"],
  data() {
    return {
      study: {},
      loaded: false,
      expanded: false,
      collapseElement: null,
      selected: false,
      pdfReports: [],
      isDownloading: false,
    };
  },
  created() {
    this.messageBus.on("selected-all", this.onSelectedStudy);
    this.messageBus.on("unselected-all", this.onUnselectedStudy);
    this.messageBus.on("added-series-to-study-" + this.studyId, () => {
      this.reloadStudy(this.studyId);
    });
    this.messageBus.on("deleted-series-from-study-" + this.studyId, () => {
      this.reloadStudy(this.studyId);
    });
    this.messageBus.on("study-updated-" + this.studyId, () => {
      this.reloadStudy(this.studyId);
    });
    this.messageBus.on(
      "study-labels-updated-in-labels-editor-" + this.studyId,
      () => {
        this.reloadStudy(this.studyId);
      },
    );
  },
  async mounted() {
    this.study = this.studies.filter((s) => s["ID"] == this.studyId)[0];
    this.loaded = true;
    this.seriesIds = this.study.Series;
    this.selected = this.selectedStudiesIds.indexOf(this.studyId) != -1;

    if (!this.$refs["study-collapsible-details"]) {
      console.log("no refs: ", studyResponse);
    }
    this.$refs["study-collapsible-details"].addEventListener(
      "show.bs.collapse",
      (e) => {
        if (e.target == e.currentTarget) {
          this.expanded = true;
        }
      },
    );
    this.$refs["study-collapsible-details"].addEventListener(
      "hide.bs.collapse",
      (e) => {
        if (e.target == e.currentTarget) {
          this.expanded = false;
        }
      },
    );

    var el = this.$refs["study-collapsible-details"];
    this.collapseElement = new bootstrap.Collapse(el, { toggle: false });

    for (const [k, v] of Object.entries(this.$route.query)) {
      if (k === "expand") {
        if (v == null || v === "study" || v === "series" || v === "instance") {
          this.collapseElement.show();
        }
      }
    }

    if (this.hasPdfReportIcon) {
      let instances = await api.getStudyInstancesExpanded(this.study.ID, [
        "SOPClassUID",
        "SeriesDate",
        "SeriesDescription",
      ]);
      for (let instance of instances) {
        if (
          instance.RequestedTags.SOPClassUID == "1.2.840.10008.5.1.4.1.1.104.1"
        ) {
          let titles = [];
          if (instance.RequestedTags.SeriesDate) {
            titles.push(
              dateHelpers.formatDateForDisplay(
                instance.RequestedTags.SeriesDate,
                this.uiOptions.DateFormat,
              ),
            );
          }
          if (instance.RequestedTags.SeriesDescription) {
            titles.push(instance.RequestedTags.SeriesDescription);
          }

          this.pdfReports.push({
            url: api.getInstancePdfUrl(instance.ID),
            title: titles.join(" - "),
            id: instance.ID,
          });
        }
      }
    }

    // 🆕 НОВАЯ СТРОКА - слушатель для закрытия dropdown при клике вне его
    document.addEventListener('click', this.handleOutsideClick);
  },
  beforeUnmount() {
    document.removeEventListener('click', this.handleOutsideClick);
  },
  methods: {
    filterByModality() {
      const modalities = this.modalitiesInStudyForDisplay;
      if (modalities && modalities !== '-') {
        this.$emit('filter-modality', modalities);
      }
    },
    // === QUICK DOWNLOAD WITH SPINNER ===
    async downloadStudyZip() {
      if (this.isDownloading) return;

      this.isDownloading = true;

      try {
        const url = this.downloadZipUrl;

        // Создаём скрытую ссылку
        const link = document.createElement('a');
        link.href = url;
        link.style.display = 'none';
        document.body.appendChild(link);
        link.click();
        document.body.removeChild(link);

        // Задержка чтобы показать что скачивание началось
        setTimeout(() => {
          this.isDownloading = false;
        }, 2000);

      } catch (error) {
        console.error('Download failed:', error);
        this.isDownloading = false;
      }
    },
    onDeletedStudy(studyId) {
      this.$emit("deletedStudy", this.studyId);
    },
    async reloadStudy(studyId) {
      // console.log("StudyItem: reloadStudy", studyId);
      await this.$store.dispatch("studies/reloadStudy", {
        studyId: studyId,
        study: await api.getStudy(studyId),
      });
      this.study = this.studies.filter((s) => s["ID"] == this.studyId)[0];
      // console.log("StudyItem: study reloaded ", this.study.Labels);
    },

    onSelectedStudy() {
      this.selected = true;
    },
    onUnselectedStudy() {
      this.selected = false;
    },
    async clickedSelect() {
      // console.log(this.studyId, this.selected);
      await this.$store.dispatch("studies/selectStudy", {
        studyId: this.studyId,
        isSelected: !this.selected,
      }); // this.selected is the value before the click
      this.selected = !this.selected;
      // console.log(this.studyId, this.selected);
    },
    // === QUICK SEND METHODS ===
    async sendToDicomModality(modality) {
      const jobId = await api.sendToDicomModality([this.study.ID], modality);
      this.$store.dispatch("jobs/addJob", {
        jobId: jobId,
        name: this.$t('send_to.button_title') + " DICOM (" + modality + ")",
      });
      this.closeSendDropdown();
    },
    async sendToOrthancPeer(peer) {
      const jobId = await api.sendToOrthancPeer([this.study.ID], peer);
      this.$store.dispatch("jobs/addJob", {
        jobId: jobId,
        name: this.$t('send_to.button_title') + " Peer (" + peer + ")",
      });
      this.closeSendDropdown();
    },
    async sendToDicomWebServer(server) {
      const jobId = await api.sendToDicomWebServer([this.study.ID], server);
      this.$store.dispatch("jobs/addJob", {
        jobId: jobId,
        name: this.$t('send_to.button_title') + " DicomWeb (" + server + ")",
      });
      this.closeSendDropdown();
    },
    closeSendDropdown() {
      // Закрыть dropdown после выбора
      const dropdown = this.$refs.sendDropdown;
      if (dropdown) {
        dropdown.classList.remove('show');
        dropdown.querySelector('.dropdown-menu')?.classList.remove('show');
      }
    },
    handleOutsideClick(event) {
      const menu = this.$refs.sendDropdownMenu;
      const dropdown = this.$refs.sendDropdown;
      if (menu && dropdown && !dropdown.contains(event.target)) {
        menu.classList.remove('show');
      }
    },
    toggleSendDropdown(event) {
      event.stopPropagation();
      const menu = this.$refs.sendDropdownMenu;
      if (menu) {
        menu.classList.toggle('show');
      }
    },
  },
  computed: {
    ...mapState({
      uiOptions: (state) => state.configuration.uiOptions,
      studies: (state) => state.studies.studies,
      studiesSourceType: (state) => state.studies.sourceType,
      selectedStudiesIds: (state) => state.studies.selectedStudiesIds,
      allLabels: (state) => state.labels.allLabels,
      orthancPeers: (state) => state.configuration.orthancPeers,
      targetDicomWebServers: (state) => state.configuration.targetDicomWebServers,
      targetDicomModalities: (state) => state.configuration.targetDicomModalities,
    }),
    modalitiesInStudyForDisplay() {
      if (this.study.RequestedTags.ModalitiesInStudy) {
        return this.study.RequestedTags.ModalitiesInStudy.split("\\").join(",");
      } else {
        return "";
      }
    },
    showLabels() {
      if (this.studiesSourceType == SourceType.LOCAL_ORTHANC) {
        return !this.expanded && this.allLabels && this.allLabels.length > 0;
      } else {
        return false;
      }
    },
    hasLabels() {
      return this.study && this.study.Labels && this.study.Labels.length > 0;
    },
    formattedPatientName() {
      return resourceHelpers.formatPatientName(
        this.study.PatientMainDicomTags.PatientName,
      );
    },
    formattedPatientBirthDate() {
      if (!this.study?.PatientMainDicomTags?.PatientBirthDate) return "";

      const date = String(this.study.PatientMainDicomTags.PatientBirthDate);


      const year = date.slice(0, 4);
      const month = date.slice(4, 6);
      const day = date.slice(6, 8);

      return `${day}.${month}.${year}`;
    },
    formattedStudyDate() {
      if (!this.study?.MainDicomTags?.StudyDate) return "";

      const date = this.study.MainDicomTags.StudyDate;
      const time = this.study?.MainDicomTags?.StudyTime;


      const year = date.slice(0, 4);
      const month = date.slice(4, 6);
      const day = date.slice(6, 8);

      let result = `${day}.${month}.${year}`;


      if (time && time.length >= 4) {
        const hours = time.slice(0, 2);
        const minutes = time.slice(2, 4);
        result += ` ${hours}:${minutes}`;
      }

      return result;
    },
    seriesCount() {
      if (
        this.study.sourceType == SourceType.REMOTE_DICOM ||
        this.study.sourceType == SourceType.REMOTE_DICOM_WEB
      ) {
        return this.study.MainDicomTags.NumberOfStudyRelatedSeries;
      } else if (this.study.Series) {
        return this.study.Series.length;
      }
    },
    instancesCount() {
      if (this.study.RequestedTags) {
        return this.study.RequestedTags.NumberOfStudyRelatedInstances;
      }
    },
    seriesAndInstancesCount() {
      const seriesCount = this.seriesCount;
      const instancesCount = this.instancesCount;
      if (instancesCount) {
        return String(seriesCount) + "/" + String(instancesCount);
      } else {
        return seriesCount;
      }
    },
    hasPdfReportIconColumn() {
      return (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        this.uiOptions.EnableReportQuickButton
      );
    },
    hasPdfReportIconPlaceholder() {
      return this.hasPdfReportIconColumn && !this.hasPdfReportIcon;
    },
    hasPdfReportIcon() {
      return (
        this.study.RequestedTags.SOPClassesInStudy &&
        this.study.RequestedTags.SOPClassesInStudy.indexOf(
          "1.2.840.10008.5.1.4.1.1.104.1",
        ) != -1 &&
        this.uiOptions.EnableReportQuickButton
      );
    },
    hasPrimaryViewerColumn() {
      return (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        this.uiOptions.EnableViewerQuickButton
      );
    },
    hasPrimaryViewerIconPlaceholder() {
      return this.hasPrimaryViewerColumn && !this.hasPrimaryViewerIcon;
    },
    hasPrimaryViewerIcon() {
      return this.hasPrimaryViewerColumn && this.primaryViewerUrl;
    },
    // === QUICK DOWNLOAD ===
    hasQuickDownloadIcon() {
      return (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        this.uiOptions.EnableDownloadQuickButton
      );
    },
    downloadZipUrl() {
      const filename = resourceHelpers.replaceResourceTagsInStringPlainText(
        this.uiOptions.DownloadStudyFileNameTemplate,
        this.study.PatientMainDicomTags,
        this.study.MainDicomTags,
        null,
        null,
        this.study.ID,
        'study'
      );
      return api.getDownloadZipUrl('study', this.study.ID, filename);
    },

    // === QUICK SEND ===
    hasQuickSendIcon() {
      return (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        this.uiOptions.EnableSendQuickButton &&
        this.hasSendToTargets
      );
    },
    hasSendToTargets() {
      return (
        this.orthancPeers.length > 0 ||
        this.targetDicomWebServers.length > 0 ||
        Object.keys(this.targetDicomModalities).length > 0
      );
    },

    // === QUICK ACTIONS COLUMN ===
    hasQuickActionsColumn() {
      return (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        (this.uiOptions.EnableDownloadQuickButton || this.uiOptions.EnableSendQuickButton)
      );
    },
    primaryViewerUrl() {
      return resourceHelpers.getPrimaryViewerUrl(
        "study",
        this.study.ID,
        this.study.MainDicomTags.StudyInstanceUID,
      );
    },
    primaryViewerTokenType() {
      return resourceHelpers.getPrimaryViewerTokenType();
    },
    colSpanStudyDetails() {
      let span = this.uiOptions.StudyListColumns.length + 1;
      if (this.hasPrimaryViewerColumn) {
        span++;
      }
      if (this.hasPdfReportIconColumn) {
        span++;
      }
      return span;
    },
  },
  components: { SeriesList, StudyDetails, TokenLinkButton },
};
</script>

<template>
  <tbody>
    <tr v-if="loaded" :class="{
      'study-row-collapsed': !expanded,
      'study-row-expanded': expanded,
      'study-row-show-labels': showLabels,
    }">
      <td>
        <div class="form-check">
          <input class="form-check-input" type="checkbox" v-model="selected" @click="clickedSelect" />
        </div>
      </td>

      <td v-if="hasPrimaryViewerIcon || hasQuickDownloadIcon || hasQuickSendIcon"
        class="td-viewer-icon td-quick-actions">
        <div class="quick-actions-group">

          <!-- Viewer Button -->
          <TokenLinkButton v-if="hasPrimaryViewerIcon && primaryViewerUrl" level="study" :linkUrl="primaryViewerUrl"
            :resourcesOrthancId="[study.ID]" linkType="icon" iconClass="bi bi-eye-fill"
            :tokenType="primaryViewerTokenType" :opensInNewTab="true" :title="$t('open_in_viewer')">
          </TokenLinkButton>

          <!-- Download Button with Spinner -->
          <button v-if="hasPrimaryViewerIcon && primaryViewerUrl" class="quick-action-btn quick-action-download"
            :class="{ 'is-downloading': isDownloading }" @click.stop="downloadStudyZip" :title="$t('download_zip')"
            :disabled="isDownloading">
            <template v-if="isDownloading">
              <span class="spinner-border spinner-border-sm" role="status"></span>
            </template>
            <template v-else>
              <i class="bi bi-download" style="padding-top: 4px;"></i>
            </template>
          </button>

          <!-- Send Dropdown Button -->
          <div v-if="hasPrimaryViewerIcon && primaryViewerUrl" class="quick-send-dropdown" ref="sendDropdown">
            <button class="quick-action-btn quick-action-send" @click.stop="toggleSendDropdown($event)"
              :title="$t('send_to.button_title')">
              <i class="bi bi-send-fill" style="padding-top: 4px;"></i>
            </button>

            <div class="quick-send-menu" ref="sendDropdownMenu">
              <!-- DICOM Modalities -->
              <div v-if="Object.keys(targetDicomModalities).length > 0" class="send-section">
                <div class="send-section-title">{{ $t('send_to.dicom') }}</div>
                <button v-for="modality in Object.keys(targetDicomModalities)" :key="'mod-' + modality"
                  class="send-menu-item" @click.stop="sendToDicomModality(modality)">
                  <i class="bi bi-display"></i>
                  {{ modality }}
                </button>
              </div>

              <!-- Orthanc Peers -->
              <div v-if="orthancPeers.length > 0" class="send-section">
                <div class="send-section-title">{{ $t('send_to.orthanc_peer') }}</div>
                <button v-for="peer in orthancPeers" :key="'peer-' + peer" class="send-menu-item"
                  @click.stop="sendToOrthancPeer(peer)">
                  <i class="bi bi-hdd-network-fill" style="padding-top: 4px;"></i>
                  {{ peer }}
                </button>
              </div>

              <!-- DicomWeb Servers -->
              <div v-if="targetDicomWebServers.length > 0" class="send-section">
                <div class="send-section-title">{{ $t('send_to.dicom_web') }}</div>
                <button v-for="server in targetDicomWebServers" :key="'dw-' + server" class="send-menu-item"
                  @click.stop="sendToDicomWebServer(server)">
                  <i class="bi bi-cloud-upload-fill" style="padding-top: 4px;"></i>
                  {{ server }}
                </button>
              </div>
            </div>
          </div>

        </div>
      </td>

      <!-- Placeholder если колонка должна быть, но кнопок нет -->
      <td v-else-if="hasPrimaryViewerColumn || hasQuickActionsColumn"></td>
      <td v-if="hasPdfReportIcon" class="td-pdf-icon">
        <TokenLinkButton v-for="pdfReport in pdfReports" :key="pdfReport.id" level="study" :linkUrl="pdfReport.url"
          :resourcesOrthancId="[study.ID]" linkType="icon" iconClass="bi bi-file-earmark-text"
          :tokenType="'download-instant-link'" :opensInNewTab="true" :title="pdfReport.title">
        </TokenLinkButton>
      </td>
      <td v-if="hasPdfReportIconPlaceholder"></td>

      <td v-for="columnTag in uiOptions.StudyListColumns" :key="columnTag" class="cut-text" :class="{
        'text-center':
          columnTag in
          [
            'modalities',
            'seriesCount',
            'instancesCount',
            'seriesAndInstancesCount',
          ],
      }" data-bs-toggle="collapse" v-bind:data-bs-target="'#study-details-' + this.studyId">
        <span v-if="columnTag == 'StudyDate'" data-bs-toggle="tooltip" v-bind:title="formattedStudyDate">{{
          formattedStudyDate
        }}
        </span>
        <span v-else-if="columnTag == 'AccessionNumber'" data-bs-toggle="tooltip"
          v-bind:title="study.MainDicomTags.AccessionNumber">{{ study.MainDicomTags.AccessionNumber }}
        </span>
        <span v-else-if="columnTag == 'PatientID'" data-bs-toggle="tooltip"
          v-bind:title="study.PatientMainDicomTags.PatientID">{{ study.PatientMainDicomTags.PatientID }}
        </span>
        <span v-else-if="columnTag == 'PatientSex'" data-bs-toggle="tooltip"
          v-bind:title="study.PatientMainDicomTags.PatientSex">{{ study.PatientMainDicomTags.PatientSex }}
        </span>
        <span v-else-if="columnTag == 'PatientName'" data-bs-toggle="tooltip" v-bind:title="formattedPatientName">{{
          formattedPatientName }}
          <span :colspan="uiOptions.StudyListColumns.length" class="label-row">
            <span v-for="label in study.Labels" :key="label" class="label badge">{{
              label
            }}</span>
            <span v-if="!hasLabels">&nbsp;</span>
          </span></span>
        <span v-else-if="columnTag == 'PatientBirthDate'" data-bs-toggle="tooltip"
          v-bind:title="formattedPatientBirthDate">{{
            formattedPatientBirthDate }}
        </span>
        <span v-else-if="columnTag == 'StudyDescription'" data-bs-toggle="tooltip"
          v-bind:title="study.MainDicomTags.StudyDescription">{{ study.MainDicomTags.StudyDescription }}
        </span>
        <span v-else-if="columnTag == 'modalities'" class="modality-badge clickable" @click.stop="filterByModality"
          :title="$t('filter_by_modality')" data-bs-toggle="tooltip" v-bind:title="modalitiesInStudyForDisplay">
          {{ modalitiesInStudyForDisplay }}
        </span>
        <span v-else-if="columnTag == 'seriesCount'" data-bs-toggle="tooltip" v-bind:title="seriesCount">{{ seriesCount
        }}
        </span>
        <span v-else-if="columnTag == 'instancesCount'" data-bs-toggle="tooltip" v-bind:title="instancesCount">{{
          instancesCount }}
        </span>
        <span v-else-if="columnTag == 'seriesAndInstancesCount'" data-bs-toggle="tooltip"
          v-bind:title="seriesAndInstancesCount">{{ seriesAndInstancesCount }}
        </span>
        <span v-else>{{ study.MainDicomTags[columnTag] }} </span>
      </td>
    </tr>
    <!-- <tr v-show="showLabels" class="tttr">

      <td v-if="hasPrimaryViewerColumn" class="1"></td>
      <td v-if="hasPdfReportIconColumn" class="2"></td>
    </tr>-->
    <tr v-show="loaded" class="study-details-row" :class="{ 'expanded': expanded }" :id="'study-details-' + studyId"
      ref="study-collapsible-details">
      <td v-if="loaded && expanded" :colspan="colSpanStudyDetails">
        <div class="study-details-content">
          <StudyDetails :studyId="studyId" :studyMainDicomTags="study.MainDicomTags"
            :patientMainDicomTags="study.PatientMainDicomTags" :labels="study.Labels" @deletedStudy="onDeletedStudy">
          </StudyDetails>
        </div>
      </td>
    </tr>
  </tbody>
</template>

<style>
/* =============================================================================
   STUDYITEM.VUE - MODERN STYLES (D-MIS Design System)
   Matching SideBar.vue and StudyList.vue styling
   ============================================================================= */

/* =============================================================================
   STUDY ROW STATES
   ============================================================================= */

/* Collapsed Row (Normal State) */
.study-row-collapsed {
  border-top: 1px solid #e2e8f0;
  transition: all 0.3s ease;
  position: relative;
}

.study-row-collapsed:hover {
  background: linear-gradient(90deg,
      rgba(59, 130, 246, 0.08) 0%,
      rgba(59, 130, 246, 0.03) 100%);
  transform: translateX(2px);
  box-shadow: inset 3px 0 0 #3b82f6;
}

/* Expanded Row (Active State) */
.study-row-expanded {
  background: linear-gradient(135deg, #f1f5f9 0%, #e2e8f0 100%) !important;
  font-weight: 600;
  border-top: 3px solid #3b82f6 !important;
  border-bottom: none !important;
  box-shadow: 0 4px 12px rgba(59, 130, 246, 0.15);
  position: relative;
  z-index: 10;
}

.study-row-expanded>td {
  padding: 14px 10px !important;
  color: #1e293b;
}

.study-row-expanded> :first-child {
  border-left: 4px solid #3b82f6;
}

.study-row-expanded:hover {
  background: linear-gradient(135deg, #e2e8f0 0%, #cbd5e1 100%) !important;
}

/* Row with Labels Visible */
.study-row-show-labels {
  border-bottom: none !important;
}

/* =============================================================================
   STUDY DETAILS EXPANDED ROW
   ============================================================================= */
.study-details-expanded {
  background: linear-gradient(180deg, #f8fafc 0%, #f1f5f9 100%) !important;
  border-top: none !important;
  border-bottom: 3px solid #3b82f6 !important;
  box-shadow: inset 0 4px 8px rgba(59, 130, 246, 0.08);
}

.study-details-expanded td {
  padding: 20px !important;
  background: #e1e1e1;
}

.study-table .series-row-expanded {
  background: #e1e1e1 !important;
  cursor: pointer;
}

.study-details-expanded:hover {
  background: linear-gradient(180deg, #f1f5f9 0%, #e2e8f0 100%) !important;
}

td .collapsed {
  cursor: pointer;
}

td .collapsed:hover {
  cursor: pointer;
  background: #f8fafc
}

/* =============================================================================
   LABELS ROW
   ============================================================================= */
.label-row {
  border-top: none !important;
  border-bottom: none !important;
  padding: 6px 10px !important;
  background: linear-gradient(90deg,
      rgba(59, 130, 246, 0.03) 0%,
      transparent 100%);
}

.label-row .label {
  display: inline-block;
  margin: 2px 4px;
  padding: 4px 10px;
  background: linear-gradient(135deg, #3b82f6 0%, #2563eb 100%);
  color: white;
  border-radius: 12px;
  font-size: 11px;
  font-weight: 500;
  letter-spacing: 0.3px;
  box-shadow: 0 2px 4px rgba(59, 130, 246, 0.2);
  transition: all 0.2s ease;
  border: none;
}

.label-row .label:hover {
  transform: translateY(-1px);
  box-shadow: 0 4px 8px rgba(59, 130, 246, 0.3);
  background: linear-gradient(135deg, #2563eb 0%, #1d4ed8 100%);
}

/* Alternative label colors for variety */
.label-row .label:nth-child(3n + 1) {
  background: linear-gradient(135deg, #8b5cf6 0%, #7c3aed 100%);
}

.label-row .label:nth-child(3n + 2) {
  background: linear-gradient(135deg, #06b6d4 0%, #0891b2 100%);
}

.label-row .label:nth-child(3n + 3) {
  background: linear-gradient(135deg, #10b981 0%, #059669 100%);
}

/* Empty labels placeholder */
.label-row span:empty::before {
  content: " a0";
  /* non-breaking space */
}

/* =============================================================================
   TABLE CELLS - CONTENT STYLING
   ============================================================================= */
.study-row-collapsed td,
.study-row-expanded td {
  vertical-align: middle;
  padding: 12px 10px;
  transition: all 0.2s ease;
}

/* Text Truncation */
.cut-text {
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  max-width: 0;
  /* Force text truncation with ellipsis */
}

/* Center alignment for specific columns */
.text-center {
  text-align: center !important;
}

/* Text with tooltip should have subtle underline on hover */
.cut-text span[data-bs-toggle="tooltip"] {
  cursor: pointer;
  position: relative;
}

.cut-text span[data-bs-toggle="tooltip"]:hover {
  color: #3b82f6;
}

.cut-text span[data-bs-toggle="tooltip"]:hover::after {
  content: "";
  position: absolute;
  bottom: -2px;
  left: 0;
  right: 0;
  height: 1px;
  background: linear-gradient(90deg, #3b82f6 0%, transparent 100%);
}

/* =============================================================================
   CHECKBOX STYLING
   ============================================================================= */
.form-check {
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 100%;
}

.form-check-input {
  width: 18px;
  height: 18px;
  border: 2px solid #cbd5e1;
  border-radius: 4px;
  cursor: pointer;
  transition: all 0.2s ease;
  margin: 0;
}

.form-check-input:hover {
  border-color: #3b82f6;
  box-shadow: 0 0 0 3px rgba(59, 130, 246, 0.1);
  transform: scale(1.05);
}

.form-check-input:checked {
  background-color: #3b82f6;
  border-color: #3b82f6;
  background-image: url("data:image/svg+xml,%3csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 20 20'%3e%3cpath fill='none' stroke='%23fff' stroke-linecap='round' stroke-linejoin='round' stroke-width='3' d='M6 10l3 3l6-6'/%3e%3c/svg%3e");
}

.form-check-input:focus {
  outline: 2px solid #3b82f6;
  outline-offset: 2px;
}

/* =============================================================================
   ICON COLUMNS
   ============================================================================= */
.td-viewer-icon,
.td-pdf-icon {
  padding: 0 !important;
  width: 40px;
  text-align: center;
  vertical-align: middle;
}

.td-viewer-icon a,
.td-pdf-icon a {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 100%;
  height: 100%;
  min-height: 40px;
  transition: all 0.2s ease;
  position: relative;
}

.td-viewer-icon .bi,
.td-pdf-icon .bi {
  font-size: 18px;
  color: #64748b;
  transition: all 0.2s ease;
}

/* Viewer Icon Hover 
.td-viewer-icon a:hover {
  background: linear-gradient(
    135deg,
    rgba(59, 130, 246, 0.1) 0%,
    rgba(59, 130, 246, 0.05) 100%
  );
}*/

.td-viewer-icon a:hover .bi {
  color: #3b82f6;
  transform: scale(1.2);
}

/* PDF Icon Hover
.td-pdf-icon a:hover {
  background: linear-gradient(
    135deg,
    rgba(239, 68, 68, 0.1) 0%,
    rgba(239, 68, 68, 0.05) 100%
  );
}

.td-pdf-icon a:hover .bi {
  color: #ef4444;
  transform: scale(1.2);
} */

/* Icon Active State */
.td-viewer-icon a:active .bi,
.td-pdf-icon a:active .bi {
  transform: scale(1.1);
}

/* Multiple PDF Reports */
.td-pdf-icon a+a {
  border-top: 1px solid #f1f5f9;
}

/* =============================================================================
   COLLAPSE ANIMATION
   ============================================================================= */
.collapse {
  transition:
    height 0.3s ease,
    opacity 0.3s ease;
}

.collapsing {
  opacity: 0.5;
}

.collapse.show {
  opacity: 1;
}

/* =============================================================================
   HOVER EFFECTS & INTERACTIONS
   ============================================================================= */

/* Clickable row indicator */
.study-row-collapsed td[data-bs-toggle="collapse"] {
  cursor: pointer;
  position: relative;
}

.study-row-collapsed td[data-bs-toggle="collapse"]:hover::before {
  content: "";
  position: absolute;
  left: 0;
  top: 0;
  bottom: 0;
  width: 3px;
  background: linear-gradient(180deg, #3b82f6 0%, #8b5cf6 100%);
  animation: slideIn 0.3s ease;
}

@keyframes slideIn {
  from {
    transform: scaleY(0);
    opacity: 0;
  }

  to {
    transform: scaleY(1);
    opacity: 1;
  }
}

/* =============================================================================
   QUICK ACTIONS (Download + Send) - Unified Column
   ============================================================================= */

.td-quick-actions {
  width: 90px;
  max-width: 100px;
  padding: 4px !important;
  vertical-align: middle;
  display: flex;
  margin: 0 5px;

}

.quick-actions-group {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 4px;
}

/* =============================================================================
   QUICK ACTION BUTTONS - Base Style (matching TokenLinkButton)
   ============================================================================= */

.quick-action-btn {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 28px;
  height: 28px;
  padding: 0;
  border: none;
  border-radius: 6px;
  background: transparent;
  color: #64748b;
  font-size: 14px;
  cursor: pointer;
  transition: all 0.2s ease;
  text-decoration: none;
}

.quick-action-btn:hover {
  transform: translateY(-1px);

}

.quick-action-btn:active {
  transform: scale(0.95);
}


/*
.quick-action-download:hover,
.quick-actions-group :deep(a[title*="download"]):hover {
  background: linear-gradient(135deg,#3b82f61a,#3b82f60d);
  color: white !important;
}



.quick-action-send:hover {
  background: linear-gradient(135deg,#3b82f61a,#3b82f60d);
  color: white;
}
*/
/* =============================================================================
   SEND DROPDOWN - Lightweight Style
   ============================================================================= */

.quick-send-dropdown {
  position: relative;
  display: inline-flex;
}

.quick-send-menu {
  position: absolute;
  top: 100%;
  right: 0;
  z-index: 1000;
  min-width: 180px;
  max-width: 240px;
  padding: 8px 0;
  margin-top: 4px;
  background: white;
  border: 1px solid #e2e8f0;
  border-radius: 10px;
  box-shadow: 0 8px 24px rgba(0, 0, 0, 0.12);
  opacity: 0;
  visibility: hidden;
  transform: translateY(-8px);
  transition: all 0.2s ease;
}

.quick-send-menu.show {
  opacity: 1;
  visibility: visible;
  transform: translateY(0);
}

/* Section Title */
.send-section {
  padding: 4px 0;
}

.send-section:not(:first-child) {
  border-top: 1px solid #f1f5f9;
  margin-top: 4px;
  padding-top: 8px;
}

.send-section-title {
  padding: 4px 12px 6px;
  font-size: 10px;
  font-weight: 600;
  color: #94a3b8;
  text-transform: uppercase;
  letter-spacing: 0.5px;
}

/* Menu Items */
.send-menu-item {
  display: flex;
  align-items: center;
  gap: 8px;
  width: 100%;
  padding: 8px 12px;
  border: none;
  background: transparent;
  color: #475569;
  font-size: 13px;
  font-weight: 500;
  text-align: left;
  cursor: pointer;
  transition: all 0.15s ease;
}

.send-menu-item:hover {
  background: linear-gradient(90deg, rgba(139, 92, 246, 0.1) 0%, rgba(139, 92, 246, 0.05) 100%);
  color: #7c3aed;
}

.send-menu-item:active {
  background: rgba(139, 92, 246, 0.15);
}

.send-menu-item i {
  font-size: 14px;
  color: #8b5cf6;
  width: 18px;
  text-align: center;
}

/* =============================================================================
   CLOSE DROPDOWN ON OUTSIDE CLICK
   ============================================================================= */

/* Backdrop for closing dropdown */
.quick-send-menu.show::before {
  content: '';
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  z-index: -1;
}

/* =============================================================================
   RESPONSIVE & COMPACT VIEW
   ============================================================================= */

@media (max-width: 1200px) {


  .quick-action-btn {
    width: 24px;
    height: 24px;
    font-size: 12px;
  }

  .quick-send-menu {
    min-width: 160px;
  }
}

/* =============================================================================
   ANIMATION FOR DROPDOWN
   ============================================================================= */

@keyframes dropdownSlideIn {
  from {
    opacity: 0;
    transform: translateY(-8px) scale(0.95);
  }

  to {
    opacity: 1;
    transform: translateY(0) scale(1);
  }
}

.quick-send-menu.show {
  animation: dropdownSlideIn 0.2s ease forwards;
}

/* =============================================================================
   RESPONSIVE IMPROVEMENTS
   ============================================================================= */
@media (max-width: 1200px) {

  .study-row-collapsed td,
  .study-row-expanded td {
    padding: 10px 6px;
    font-size: 12px;
  }

  .label-row .label {
    font-size: 10px;
    padding: 3px 8px;
    margin: 1px 2px;
  }
}

@media (max-width: 768px) {

  .td-viewer-icon,
  .td-pdf-icon {
    width: 30px;
  }

  .td-viewer-icon .bi,
  .td-pdf-icon .bi {
    font-size: 16px;
  }
}

/* =============================================================================
   ACCESSIBILITY IMPROVEMENTS
   ============================================================================= */

/* Focus visible for keyboard navigation */
.study-row-collapsed:focus-within,
.study-row-expanded:focus-within {
  outline: 2px solid #3b82f6;
  outline-offset: -2px;
}

/* High contrast for better readability */
@media (prefers-contrast: high) {
  .study-row-collapsed {
    border-top-width: 2px;
  }

  .study-row-expanded {
    border-top-width: 4px !important;
  }

  .label-row .label {
    border: 1px solid rgba(255, 255, 255, 0.3);
  }
}

/* Reduced motion for accessibility */
@media (prefers-reduced-motion: reduce) {

  .study-row-collapsed,
  .study-row-expanded,
  .study-details-expanded,
  .label-row .label,
  .form-check-input,
  .td-viewer-icon a,
  .td-pdf-icon a {
    transition: none;
    animation: none;
  }
}

/* =============================================================================
   LOADING STATE
   ============================================================================= */
.study-row-collapsed.loading td {
  background: linear-gradient(90deg, #f1f5f9 25%, #e2e8f0 50%, #f1f5f9 75%);
  background-size: 200% 100%;
  animation: skeleton-loading 1.5s ease-in-out infinite;
}

@keyframes skeleton-loading {
  0% {
    background-position: 200% 0;
  }

  100% {
    background-position: -200% 0;
  }
}

/* =============================================================================
   SELECTED STATE INDICATOR
   ============================================================================= */
.study-row-collapsed:has(.form-check-input:checked),
.study-row-expanded:has(.form-check-input:checked) {
  background: linear-gradient(90deg,
      rgba(59, 130, 246, 0.05) 0%,
      transparent 100%);
  border-left: 3px solid #3b82f6;
}

.study-row-collapsed:has(.form-check-input:checked):hover {
  background: linear-gradient(90deg,
      rgba(59, 130, 246, 0.1) 0%,
      rgba(59, 130, 246, 0.05) 100%);
}

/* =============================================================================
   PRINT STYLES
   ============================================================================= */
@media print {

  .study-row-collapsed,
  .study-row-expanded {
    border: 1px solid #000 !important;
    box-shadow: none !important;
    background: white !important;
  }

  .study-row-collapsed:hover,
  .study-row-expanded:hover {
    background: white !important;
    transform: none !important;
  }

  .form-check-input {
    border: 1px solid #000;
  }

  /* =============================================================================
   QUICK ACTIONS BUTTONS
   ============================================================================= */

  .quick-actions-group {
    display: flex;
    align-items: center;
    gap: 4px;
  }

  .quick-action-btn {
    width: 28px;
    height: 28px;
    padding: 0;
    border: none;
    border-radius: 6px;
    background: transparent;
    color: #64748b;
    font-size: 14px;
    cursor: pointer;
    display: flex;
    align-items: center;
    justify-content: center;
    transition: all 0.2s ease;
  }

  .quick-action-btn:hover {
    transform: translateY(-1px);
  }

  .quick-action-btn:disabled {
    cursor: wait;
    opacity: 0.8;
  }

  /* Download Button - Green */
  .quick-action-download {
    color: #10b981;
    margin: 0px 4px;
  }

  .quick-action-download:hover {
    background: linear-gradient(135deg, #10b981 0%, #059669 100%);
    color: white;
    box-shadow: 0 2px 8px rgba(16, 185, 129, 0.3);
  }

  .quick-action-download.is-downloading {
    background: linear-gradient(135deg, #10b981 0%, #059669 100%);
    color: white;
  }

  .quick-action-download .spinner-border-sm {
    width: 14px;
    height: 14px;
    border-width: 2px;
  }

  /* Send Button - Purple */
  .quick-action-send {
    color: #8b5cf6;
  }

  .quick-action-send:hover {
    background: linear-gradient(135deg, #8b5cf6 0%, #7c3aed 100%);
    color: white;
    box-shadow: 0 2px 8px rgba(139, 92, 246, 0.3);
  }

  /* =============================================================================
   SEND DROPDOWN MENU
   ============================================================================= */

  .quick-send-dropdown {
    position: relative;
  }

  .quick-send-menu {
    position: absolute;
    top: 100%;
    right: 0;
    z-index: 1000;
    min-width: 180px;
    max-width: 240px;
    padding: 8px 0;
    margin-top: 4px;
    background: white;
    border: 1px solid #e2e8f0;
    border-radius: 10px;
    box-shadow: 0 8px 24px rgba(0, 0, 0, 0.12);
    opacity: 0;
    visibility: hidden;
    transform: translateY(-8px);
    transition: all 0.2s ease;
  }

  .quick-send-menu.show {
    opacity: 1;
    visibility: visible;
    transform: translateY(0);
  }

  /* Send Sections */
  .send-section {
    padding: 4px 0;
  }

  .send-section:not(:last-child) {
    border-bottom: 1px solid #f1f5f9;
  }

  .send-section-title {
    padding: 6px 12px;
    font-size: 10px;
    font-weight: 600;
    text-transform: uppercase;
    color: #94a3b8;
    letter-spacing: 0.5px;
  }

  /* Send Menu Items */
  .send-menu-item {
    display: flex;
    align-items: center;
    gap: 8px;
    width: 100%;
    padding: 8px 12px;
    border: none;
    background: transparent;
    color: #475569;
    font-size: 13px;
    font-weight: 500;
    text-align: left;
    cursor: pointer;
    transition: all 0.15s ease;
  }

  .send-menu-item:hover {
    background: linear-gradient(90deg, rgba(139, 92, 246, 0.1) 0%, rgba(139, 92, 246, 0.05) 100%);
    color: #7c3aed;
  }

  .send-menu-item i {
    font-size: 14px;
    color: #8b5cf6;
    width: 18px;
    flex-shrink: 0;
  }

  .td-viewer-icon,
  .td-pdf-icon {
    display: none;
  }
}

.td-quick-actions button:hover .bi {
  color: #3b82f6;
  transform: scale(1.2)
}

th.download-buttons-group {
  width: 20;
}

.div-icon {
  background: #0f172a1a;
  border-radius: 6px;
  height: 32px;
  width: 32px;
  color: white;
  padding-bottom: 4px;
}

button.quick-action-btn.quick-action-download {
  background: #0f172a1a;
  border-radius: 6px;
  height: 32px;
  width: 32px;
  color: white;
  padding-bottom: 4px;
}

button.quick-action-btn.quick-action-send {
  background: #0f172a1a;
  border-radius: 6px;
  height: 32px;
  width: 32px;
  color: white;
  padding-bottom: 4px;
}

button.form-control.study-list-filter.btn.filter-button.btn-sm {
  min-width: 30px;
}

.modality-badge.clickable {
  cursor: pointer;
  transition: all 0.2s ease;
  color: #3b82f6;
  font-weight: 500;
}

.modality-badge.clickable:hover {
  opacity: 0.8;
  text-decoration: underline;
}

/* Стили для строки деталей */
.study-details-row {
  opacity: 0;
  transform: translateY(-10px);
  transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
  overflow: hidden;
}

.study-details-row.expanded {
  opacity: 1;
  transform: translateY(0);
}

/* Контейнер контента для дополнительной анимации */
.study-details-content {
  animation: fadeSlideIn 0.35s ease-out;
  padding: 16px;
  background: #f8fafc;
  border-radius: 0 0 8px 8px;
}

@keyframes fadeSlideIn {
  from {
    opacity: 0;
    transform: translateY(-15px);
  }

  to {
    opacity: 1;
    transform: translateY(0);
  }
}

/* Альтернативная анимация высоты если нужно */
.study-details-row {
  max-height: 0;
  transition: max-height 0.4s ease-out, opacity 0.3s ease, transform 0.3s ease;
}

.study-details-row.expanded {
  max-height: 2000px;
  /* Достаточно большое значение */
  opacity: 1;
  transform: translateY(0);
}

/* =============================================================================
   RESPONSIVE
   ============================================================================= */

@media (max-width: 1200px) {
  .quick-action-btn {
    width: 24px;
    height: 24px;
    font-size: 12px;
  }

  .quick-action-download .spinner-border-sm {
    width: 12px;
    height: 12px;
  }

  .quick-send-menu {
    min-width: 160px;
  }
}


@media (min-width: 1024px) {
  th.download-buttons-group {
    width: 14%;
    max-width: 90px !important;
  }
}

@media (min-width: 1280px) {
  th.download-buttons-group {
    width: 12%;
    max-width: 90px !important;
  }
}

@media (min-width: 1580px) {
  th.download-buttons-group {
    width: 8%;
    max-width: 90px !important;
  }
}

@media (min-width: 1920px) {
  th.download-buttons-group {
    width: 6.5%;
    max-width: 90px !important;
  }
}

@media (min-width: 2400px) {
  th.download-buttons-group {
    width: 5%;
    max-width: 90px !important;
  }
}
</style>
