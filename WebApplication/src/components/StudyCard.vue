<script>
import SeriesList from "./SeriesList.vue";
import StudyDetails from "./StudyDetails.vue";
import { mapState } from "vuex";
import bootstrap from "bootstrap/dist/js/bootstrap.bundle.min.js";
import api from "../orthancApi";
import dateHelpers from "../helpers/date-helpers";
import SourceType from "../helpers/source-type";
import resourceHelpers from "../helpers/resource-helpers";

export default {
  props: ["studyId"],
  emits: ["deletedStudy", "filter-modality"],
  data() {
    return {
      study: null, // начинаем с null
      loaded: false,
      expanded: false,
      collapseElement: null,
      selected: false,
      pdfReports: [],
      pdfReportsLoaded: false,
      sendDropdownOpen: false,
    };
  },
  created() {
    this.messageBus.on("selected-all", this.onSelectedStudy);
    this.messageBus.on("unselected-all", this.onUnselectedStudy);
    this.messageBus.on("collapse-all-cards", this.onCollapseCard);

    // Единый обработчик для всех событий обновления
    const reloadHandler = () => {
      this.reloadStudy(this.studyId);
    };

    this._studyEventHandlers = [
      ["added-series-to-study-" + this.studyId, reloadHandler],
      ["deleted-series-from-study-" + this.studyId, reloadHandler],
      ["study-updated-" + this.studyId, reloadHandler],
      ["study-labels-updated-in-labels-editor-" + this.studyId, reloadHandler],
    ];

    for (const [event, handler] of this._studyEventHandlers) {
      this.messageBus.on(event, handler);
    }
  },
  async mounted() {
    // Получаем study из store как в StudyItem
    const studyFromStore = this.studies.filter((s) => s["ID"] == this.studyId)[0];

    if (!studyFromStore) {
      console.warn("Study not found in store:", this.studyId);
      return;
    }

    this.study = studyFromStore;
    this.loaded = true;
    this.selected = this.selectedStudiesIds.indexOf(this.studyId) != -1;

    await this.$nextTick();

    // Проверяем наличие ref перед использованием
    if (this.$refs["card-collapsible-details"]) {
      this.initCollapseElement();
    }

    document.addEventListener("click", this.handleOutsideClick);

    // Загружаем PDF отчеты если нужно
    if (this.hasPdfReportIcon) {
      await this.loadPdfReports();
    }
  },
  beforeUnmount() {
    document.removeEventListener("click", this.handleOutsideClick);
    this.messageBus.off("collapse-all-cards", this.onCollapseCard);

    if (this._studyEventHandlers) {
      for (const [event, handler] of this._studyEventHandlers) {
        this.messageBus.off(event, handler);
      }
    }

    this.messageBus.off("selected-all", this.onSelectedStudy);
    this.messageBus.off("unselected-all", this.onUnselectedStudy);
  },
  watch: {
    // Следим за изменениями study в store (как в SeriesList)
    studies: {
      deep: true,
      handler(newStudies) {
        const updatedStudy = newStudies.find((s) => s["ID"] == this.studyId);
        if (updatedStudy) {
          this.study = updatedStudy;
        }
      }
    }
  },
  methods: {
    initCollapseElement() {
      const detailsRef = this.$refs["card-collapsible-details"];
      if (!detailsRef) return;

      detailsRef.addEventListener("show.bs.collapse", (e) => {
        if (e.target == e.currentTarget) {
          this.messageBus.emit("collapse-all-cards", this.studyId);
          this.expanded = true;
          this.loadPdfReportsIfNeeded();
          this.scrollToCard();
        }
      });

      detailsRef.addEventListener("hide.bs.collapse", (e) => {
        if (e.target == e.currentTarget) {
          this.expanded = false;
        }
      });

      this.collapseElement = new bootstrap.Collapse(detailsRef, { toggle: false });
    },

    onCollapseCard(exceptStudyId) {
      if (this.studyId !== exceptStudyId && this.expanded && this.collapseElement) {
        this.collapseElement.hide();
      }
    },

    scrollToCard() {
      if (!this.$el) return;
      setTimeout(() => {
        if (!this.$el) return;
        const headerOffset = 150;
        const elementPosition = this.$el.getBoundingClientRect().top;
        const offsetPosition = elementPosition + window.pageYOffset - headerOffset;
        window.scrollTo({ top: Math.max(0, offsetPosition), behavior: 'smooth' });
      }, 350);
    },

    async loadPdfReportsIfNeeded() {
      if (this.pdfReportsLoaded || !this.hasPdfReportIcon) return;
      await this.loadPdfReports();
    },

    async loadPdfReports() {
      this.pdfReportsLoaded = true;
      try {
        let instances = await api.getStudyInstancesExpanded(this.study.ID, [
          "SOPClassUID", "SeriesDate", "SeriesDescription",
        ]);

        this.pdfReports = []; // очищаем перед загрузкой

        for (let instance of instances) {
          if (instance.RequestedTags?.SOPClassUID == "1.2.840.10008.5.1.4.1.1.104.1") {
            let titles = [];
            if (instance.RequestedTags.SeriesDate) {
              titles.push(dateHelpers.formatDateForDisplay(
                instance.RequestedTags.SeriesDate, this.uiOptions.DateFormat
              ));
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
      } catch (error) {
        console.warn("Failed to load PDF reports:", error);
      }
    },

    filterByModality() {
      const modalities = this.modalitiesInStudyForDisplay;
      if (modalities && modalities !== '-') {
        this.$emit('filter-modality', modalities);
      }
    },

    toggleCard() {
      if (this.collapseElement) {
        this.collapseElement.toggle();
      }
    },

    onDeletedStudy(studyId) {
      this.$emit("deletedStudy", this.studyId);
    },

    async reloadStudy(studyId) {
      try {
        const freshStudy = await api.getStudy(studyId);
        if (!freshStudy) {
          console.warn("Failed to reload study - not found:", studyId);
          return;
        }

        // Обновляем store (как в StudyItem)
        await this.$store.dispatch("studies/reloadStudy", {
          studyId: studyId,
          study: freshStudy,
        });

        // study обновится через watch на studies
      } catch (error) {
        console.warn("Failed to reload study:", error);
      }
    },

    onSelectedStudy() {
      this.selected = true;
    },

    onUnselectedStudy() {
      this.selected = false;
    },

    async clickedSelect() {
      await this.$store.dispatch("studies/selectStudy", {
        studyId: this.studyId,
        isSelected: !this.selected,
      });
      this.selected = !this.selected;
    },

    async sendToDicomModality(modality) {
      const jobId = await api.sendToDicomModality([this.study.ID], modality);
      this.$store.dispatch("jobs/addJob", {
        jobId: jobId,
        name: "Send to DICOM (" + modality + ")",
      });
      this.sendDropdownOpen = false;
    },

    async sendToOrthancPeer(peer) {
      const jobId = await api.sendToOrthancPeer([this.study.ID], peer);
      this.$store.dispatch("jobs/addJob", {
        jobId: jobId,
        name: "Send to Peer (" + peer + ")",
      });
      this.sendDropdownOpen = false;
    },

    async sendToDicomWebServer(server) {
      const jobId = await api.sendToDicomWebServer([this.study.ID], server);
      this.$store.dispatch("jobs/addJob", {
        jobId: jobId,
        name: "Send to DicomWeb (" + server + ")",
      });
      this.sendDropdownOpen = false;
    },

    handleOutsideClick(event) {
      const dropdown = this.$refs.sendDropdown;
      if (dropdown && !dropdown.contains(event.target)) {
        this.sendDropdownOpen = false;
      }
    },

    toggleSendDropdown(event) {
      event.stopPropagation();
      this.sendDropdownOpen = !this.sendDropdownOpen;
    },

    getLabelColor(label) {
      let hash = 0;
      for (let i = 0; i < label.length; i++) {
        const char = label.charCodeAt(i);
        hash = ((hash << 5) - hash) + char;
        hash = hash & hash;
      }
      hash = Math.abs(hash);
      const hue = hash % 360;
      const saturation = 65 + (hash % 15);
      const lightness = 45 + (hash % 10);
      return `hsl(${hue}, ${saturation}%, ${lightness}%)`;
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
      if (this.study?.RequestedTags?.ModalitiesInStudy) {
        return this.study.RequestedTags.ModalitiesInStudy.split("\\").join(", ");
      }
      return "-";
    },

    showLabels() {
      if (this.studiesSourceType == SourceType.LOCAL_ORTHANC) {
        return this.allLabels && this.allLabels.length > 0;
      }
      return false;
    },

    hasLabels() {
      return this.study?.Labels && this.study.Labels.length > 0;
    },

    formattedPatientName() {
      if (!this.study?.PatientMainDicomTags?.PatientName) return "";
      return resourceHelpers.formatPatientName(this.study.PatientMainDicomTags.PatientName);
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

    patientAge() {
      const birthDate = this.study?.PatientMainDicomTags?.PatientBirthDate;
      const studyDate = this.study?.MainDicomTags?.StudyDate;
      if (birthDate && studyDate) {
        const birth = new Date(birthDate.slice(0, 4), birthDate.slice(4, 6) - 1, birthDate.slice(6, 8));
        const study = new Date(studyDate.slice(0, 4), studyDate.slice(4, 6) - 1, studyDate.slice(6, 8));
        return Math.floor((study - birth) / (1000 * 60 * 60 * 24 * 365.25));
      }
      return "";
    },

    seriesCount() {
      if (!this.study) return 0;
      if (this.study.sourceType == SourceType.REMOTE_DICOM || this.study.sourceType == SourceType.REMOTE_DICOM_WEB) {
        return this.study.MainDicomTags?.NumberOfStudyRelatedSeries || 0;
      } else if (this.study.Series) {
        return this.study.Series.length;
      }
      return 0;
    },

    instancesCount() {
      if (!this.study?.RequestedTags) return 0;
      return this.study.RequestedTags.NumberOfStudyRelatedInstances || 0;
    },

    hasPdfReportIcon() {
      return (
        this.study?.RequestedTags?.SOPClassesInStudy &&
        this.study.RequestedTags.SOPClassesInStudy.indexOf("1.2.840.10008.5.1.4.1.1.104.1") != -1 &&
        this.uiOptions?.EnableReportQuickButton
      );
    },

    hasPrimaryViewerColumn() {
      return (
        this.studiesSourceType == SourceType.LOCAL_ORTHANC &&
        this.uiOptions?.EnableViewerQuickButton
      );
    },

    hasPrimaryViewerIcon() {
      return this.hasPrimaryViewerColumn && this.primaryViewerUrl;
    },

    primaryViewerUrl() {
      if (this.studiesSourceType == SourceType.LOCAL_ORTHANC && this.study?.ID && this.study?.MainDicomTags?.StudyInstanceUID) {
        return resourceHelpers.getPrimaryViewerUrl("study", this.study.ID, this.study.MainDicomTags.StudyInstanceUID);
      }
      return null;
    },

    downloadZipUrl() {
      if (this.study?.ID && this.study?.PatientMainDicomTags && this.study?.MainDicomTags) {
        const filename = resourceHelpers.replaceResourceTagsInStringPlainText(
          this.uiOptions?.DownloadStudyFileNameTemplate || '{PatientID}-{PatientName}',
          this.study.PatientMainDicomTags,
          this.study.MainDicomTags,
          null, null, this.study.ID, 'study'
        );
        return api.getDownloadZipUrl('study', this.study.ID, filename);
      }
      return null;
    },

    hasSendOptions() {
      return (
        (this.orthancPeers && this.orthancPeers.length > 0) ||
        (this.targetDicomWebServers && this.targetDicomWebServers.length > 0) ||
        (this.targetDicomModalities && Object.keys(this.targetDicomModalities).length > 0)
      );
    },
  },
  components: { SeriesList, StudyDetails },
};
</script>

<template>
  <div v-if="study && study.ID && loaded" class="study-card" :class="{ expanded: expanded, selected: selected }">
    <!-- Card Header -->
    <div class="card-header" @click="clickedSelect">
      <div class="card-top-row">
        <div class="card-date">
          <i class="bi bi-calendar3"></i>
          {{ formattedStudyDate }}
        </div>
        <input type="checkbox" class="card-checkbox" :checked="selected" @click.stop="clickedSelect" />
      </div>

      <div class="patient-row">
        <span class="patient-name">{{ formattedPatientName }}</span>
        <span class="patient-id">{{ study.PatientMainDicomTags?.PatientID }}</span>
        <span v-if="patientAge" class="patient-age">{{ patientAge }}y</span>
      </div>

      <div class="study-row">
        {{ study.MainDicomTags?.StudyDescription || $t("no_description") }}
      </div>

      <div class="meta-labels-row">
        <span class="modality-badge clickable" @click.stop="filterByModality" :title="$t('filter_by_modality')">
          {{ modalitiesInStudyForDisplay }}
        </span>
        <span class="count-badge">{{ seriesCount }}s / {{ instancesCount }}i</span>

        <template v-if="showLabels && hasLabels">
          <span v-for="label in study.Labels.slice(0, 2)" :key="label" class="label-badge"
            :style="{ backgroundColor: getLabelColor(label) }" :title="label">
            {{ label }}
          </span>
          <span v-if="study.Labels.length > 2" class="more-labels">+{{ study.Labels.length - 2 }}</span>
        </template>
      </div>
    </div>

    <div class="actions-row" @click.stop>
      <a v-if="hasPrimaryViewerIcon" :href="primaryViewerUrl" class="icon-btn view" target="_blank"
        :title="$t('open_in_viewer')">
        <i class="bi bi-eye"></i>
      </a>

      <a v-if="downloadZipUrl" :href="downloadZipUrl" class="icon-btn download" :title="$t('download_zip')">
        <i class="bi bi-download"></i>
      </a>

      <div class="send-wrapper" ref="sendDropdown" v-if="hasSendOptions">
        <button class="icon-btn send" @click.stop="toggleSendDropdown" :title="$t('send_to.dicom')">
          <i class="bi bi-send"></i>
        </button>

        <div class="send-menu" :class="{ show: sendDropdownOpen }">
          <div v-if="targetDicomModalities && Object.keys(targetDicomModalities).length > 0" class="menu-section">
            <div class="menu-title">DICOM</div>
            <div v-for="modality in Object.keys(targetDicomModalities)" :key="modality" class="menu-item"
              @click.stop="sendToDicomModality(modality)">
              {{ modality }}
            </div>
          </div>
          <div v-if="orthancPeers && orthancPeers.length > 0" class="menu-section">
            <div class="menu-title">Peers</div>
            <div v-for="peer in orthancPeers" :key="peer" class="menu-item" @click.stop="sendToOrthancPeer(peer)">
              {{ peer }}
            </div>
          </div>
          <div v-if="targetDicomWebServers && targetDicomWebServers.length > 0" class="menu-section">
            <div class="menu-title">DicomWeb</div>
            <div v-for="server in targetDicomWebServers" :key="server" class="menu-item"
              @click.stop="sendToDicomWebServer(server)">
              {{ server }}
            </div>
          </div>
        </div>
      </div>

      <button class="icon-btn expand" @click.stop="toggleCard" :title="expanded ? $t('collapse') : $t('expand')">
        <i class="bi" :class="expanded ? 'bi-chevron-up' : 'bi-chevron-down'"></i>
      </button>
    </div>

    <!-- Collapsible Details -->
    <div ref="card-collapsible-details" class="collapse card-details" :id="'card-details-' + studyId">
      <div v-if="expanded && study" class="details-content">
        <!-- Передаём :key для принудительного обновления при изменении labels -->
        <StudyDetails :key="'details-' + studyId + '-' + (study.Labels?.join(',') || '')" :studyId="studyId"
          :studyMainDicomTags="study.MainDicomTags" :patientMainDicomTags="study.PatientMainDicomTags"
          :labels="study.Labels" @deletedStudy="onDeletedStudy" />
        <SeriesList :key="'series-' + studyId" :studyId="studyId" :patientMainDicomTags="study.PatientMainDicomTags"
          :studyMainDicomTags="study.MainDicomTags" :studySeries="study.Series || []" @deletedStudy="onDeletedStudy" />
      </div>
    </div>
  </div>
</template>



<style scoped>
/* =============================================================================
   COMPACT STUDY CARD - 5 cards per row on 1920px
   ============================================================================= */

.study-card {
  background: white;
  border-radius: 8px;
  border: 1px solid #e2e8f0;
  transition: all 0.2s ease;
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.05);
  font-size: 11px;
  line-height: 1.3;
  display: flex;
  flex-direction: column;
}

.study-card:hover {
  border-color: #3b82f6;
  box-shadow: 0 4px 12px rgba(59, 130, 246, 0.15);
}

.study-card.selected {
  border-color: #3b82f6;
  background: #f8faff;
  box-shadow: 0 0 0 2px rgba(59, 130, 246, 0.2);
}

/* EXPANDED - Full width */
.study-card.expanded {
  grid-column: 1 / -1 !important;
  border-color: #8b5cf6;
  box-shadow: 0 8px 24px rgba(139, 92, 246, 0.2);
}

/* =============================================================================
   CARD HEADER
   ============================================================================= */

.card-header {
  padding: 10px;
  cursor: pointer;
  flex: 1;
  text-align: left;
}

/* Row 1: Date + Checkbox */
.card-top-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 4px;
}

.modality-badge.clickable {
  cursor: pointer;
  transition: all 0.2s ease;
}

.modality-badge.clickable:hover {
  opacity: 0.8;
  text-decoration: underline;
  transform: translateY(-1px);
}

.card-date {
  font-size: 10px;
  color: #64748b;
  display: flex;
  align-items: center;
  gap: 4px;
  font-weight: 500;
}

.card-date i {
  font-size: 9px;
  color: #94a3b8;
}

.card-checkbox {
  width: 14px;
  height: 14px;
  cursor: pointer;
  accent-color: #3b82f6;
}

/* Row 2: Patient */
.patient-row {
  display: flex;
  align-items: baseline;
  gap: 6px;
  margin-bottom: 2px;
  min-width: 0;
}

.patient-name {
  font-weight: 600;
  color: #1e293b;
  font-size: 12px;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  flex: 1;
  min-width: 0;
}

.patient-id {
  color: #64748b;
  font-size: 10px;
  font-family: monospace;
  white-space: nowrap;
}

.patient-age {
  color: #94a3b8;
  font-size: 10px;
  white-space: nowrap;
}

/* Row 3: Study Description */
.study-row {
  color: #475569;
  font-size: 11px;
  margin-bottom: 6px;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

/* Row 4: Meta + Labels - UNIFIED ROW */
.meta-labels-row {
  display: flex;
  align-items: center;
  gap: 4px;
  flex-wrap: wrap;
}

.modality-badge {
  background: #dbeafe;
  color: #1e40af;
  padding: 2px 6px;
  border-radius: 4px;
  font-size: 10px;
  font-weight: 600;
}

.count-badge {
  background: #f1f5f9;
  color: #64748b;
  padding: 2px 6px;
  border-radius: 4px;
  font-size: 10px;
  font-weight: 500;
}

.label-badge {
  color: white;
  padding: 2px 6px;
  border-radius: 4px;
  font-size: 9px;
  font-weight: 500;
  max-width: 70px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.more-labels {
  font-size: 9px;
  color: #94a3b8;
  font-weight: 500;
}

/* =============================================================================
   ACTIONS ROW
   ============================================================================= */

.actions-row {
  display: flex;
  align-items: center;
  gap: 2px;
  padding: 6px 10px;
  border-top: 1px solid #f1f5f9;
  background: #fafbfc;
  border-radius: 0 0 8px 8px;
}

.icon-btn {
  width: 26px;
  height: 26px;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 4px;
  border: none;
  background: transparent;
  color: #64748b;
  cursor: pointer;
  text-decoration: none;
  transition: all 0.15s ease;
  font-size: 12px;
}

.icon-btn:hover {
  background: #e2e8f0;
  color: #334155;
}

.icon-btn.view:hover {
  background: #dbeafe;
  color: #2563eb;
}

.icon-btn.download:hover {
  background: #d1fae5;
  color: #059669;
}

.icon-btn.send:hover {
  background: #ede9fe;
  color: #7c3aed;
}

.icon-btn.expand {
  margin-left: auto;
}

/* Send Dropdown */
.send-wrapper {
  position: relative;
}

.send-menu {
  position: absolute;
  bottom: calc(100% + 4px);
  left: 0;
  min-width: 140px;
  max-height: 250px;
  overflow-y: auto;
  background: white;
  border: 1px solid #e2e8f0;
  border-radius: 6px;
  box-shadow: 0 8px 20px rgba(0, 0, 0, 0.15);
  opacity: 0;
  visibility: hidden;
  transform: translateY(4px);
  transition: all 0.15s ease;
  z-index: 9999;
}

.send-menu.show {
  opacity: 1;
  visibility: visible;
  transform: translateY(0);
}

.menu-section {
  padding: 4px 0;
}

.menu-section:not(:last-child) {
  border-bottom: 1px solid #f1f5f9;
}

.menu-title {
  padding: 3px 10px;
  font-size: 9px;
  font-weight: 700;
  text-transform: uppercase;
  color: #94a3b8;
}

.menu-item {
  padding: 5px 10px;
  font-size: 11px;
  color: #475569;
  cursor: pointer;
}

.menu-item:hover {
  background: #f8fafc;
  color: #334155;
}

/* =============================================================================
   DETAILS SECTION - FULL WIDTH
   ============================================================================= */

.card-details {
  border-top: 1px solid #e2e8f0;
}

.details-content {
  padding: 16px;
  background: #f8fafc;
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.details-content>* {
  width: 100%;
}

.top-row {
  max-width: 50%;
}

.studies-cards-grid .study-card .details-content>div {
  max-width: 97%;
}

/* Анимация раскрытия карточки */
.study-card {
  transition: all 0.3s ease-in-out;
  position: relative;
  z-index: 1;
}

.study-card.expanded {
  z-index: 100;
  transform: scale(1.02);
  box-shadow: 0 8px 25px rgba(0, 0, 0, 0.25);
}

.study-card-loading,
.study-card-error {
  padding: 20px;
  text-align: center;
  color: #666;
}

.no-series-message {
  padding: 20px;
  text-align: center;
  color: #999;
  font-style: italic;
}

/* Плавная анимация для содержимого */
.card-details {
  transition: max-height 0.35s ease-in-out, opacity 0.3s ease-in-out;
  overflow: hidden;
}

.card-details.collapsing {
  transition: height 0.35s ease-in-out;
}

.card-details .details-content {
  animation: fadeSlideIn 0.3s ease-out;
}

@keyframes fadeSlideIn {
  from {
    opacity: 0;
    transform: translateY(-10px);
  }

  to {
    opacity: 1;
    transform: translateY(0);
  }
}

/* Затемнение остальных карточек при раскрытии */
.studies-cards-grid:has(.study-card.expanded) .study-card:not(.expanded) {
  opacity: 0.7;
  filter: brightness(0.95);
  transition: all 0.3s ease-in-out;
}
</style>