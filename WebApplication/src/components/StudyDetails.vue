<script>
import SeriesItem from "./SeriesItem.vue";
import SeriesList from "./SeriesList.vue";
import { mapState, mapGetters } from "vuex";
import ResourceButtonGroup from "./ResourceButtonGroup.vue";
import ResourceDetailText from "./ResourceDetailText.vue";
import AuditLogs from "./AuditLogs.vue";
import api from "../orthancApi";
import LabelsEditor from "./LabelsEditor.vue";
import SourceType from "../helpers/source-type";

export default {
  props: ["studyId", "studyMainDicomTags", "patientMainDicomTags", "labels"],
  emits: ["deletedStudy"],
  setup() { },
  data() {
    return {
      samePatientStudiesCount: 0,
      studySeries: [],
      hasLoadedSamePatientsStudiesCount: false,
      allLabelsLocalCopy: new Set(),
      studyMainDicomTagsLocalCopy: {},
      remoteStudyFoundLocally: false,
      labelsComponentKey: 0,
    };
  },
  async created() {
    this.allLabelsLocalCopy = await api.loadAllLabels();
    this.messageBus.on(
      "added-series-to-study-" + this.studyId,
      this.reloadSeriesList,
    );
  },
  async mounted() {
    this.samePatientStudiesCount = (
      await api.getSamePatientStudies(
        this.patientMainDicomTags,
        this.uiOptions.ShowSamePatientStudiesFilter,
      )
    ).length;
    this.studyMainDicomTagsLocalCopy = { ...this.studyMainDicomTags };
    await this.reloadSeriesList();
    this.hasLoadedSamePatientsStudiesCount = true;

    if (
      this.studiesSourceType == SourceType.REMOTE_DICOM ||
      this.studiesSourceType == SourceType.REMOTE_DICOM_WEB
    ) {
      this.remoteStudyFoundLocally = await api.studyExists(
        this.studyMainDicomTagsLocalCopy.StudyInstanceUID,
      );
    }

    if (this.studiesSourceType == SourceType.LOCAL_ORTHANC) {
      for (const t of this.uiOptions.StudyMainTags) {
        if (
          !(t in this.studyMainDicomTagsLocalCopy) &&
          ["VisitComments"].includes(t)
        ) {
          console.info(
            "Possibly missing tag from DB: " +
            t +
            ", will get it from an instance",
          );
          let instances = await api.getStudyInstancesIds(this.studyId);
          let instanceTags = await api.getInstanceTags(instances[0]);
          for (const [k, v] of Object.entries(instanceTags)) {
            if (v["Name"] == t) {
              this.studyMainDicomTagsLocalCopy[t] = v["Value"];
            }
          }
        }
      }
    }
  },
  computed: {
    ...mapState({
      uiOptions: (state) => state.configuration.uiOptions,
      allLabels: (state) => state.labels.allLabels,
      studiesSourceType: (state) => state.studies.sourceType,
      studiesRemoteSource: (state) => state.studies.remoteSource,
    }),
    showLabels() {
      if (this.studiesSourceType == SourceType.LOCAL_ORTHANC) {
        return true;
      } else {
        return false;
      }
    },
    samePatientStudiesLink() {
      let filters = [];
      for (let tag of this.uiOptions.ShowSamePatientStudiesFilter) {
        if (tag in this.patientMainDicomTags) {
          if (["PatientBirthDate"].indexOf(tag) >= 0) {
            filters.push(tag + "=" + this.patientMainDicomTags[tag] + "");
          } else {
            filters.push(tag + '="' + this.patientMainDicomTags[tag] + '"');
          }
        }
      }
      return "/filtered-studies?" + filters.join("&");
    },
    isLocalOrthanc() {
      return this.studiesSourceType == SourceType.LOCAL_ORTHANC;
    },
    isRemoteSource() {
      return (
        this.studiesSourceType == SourceType.REMOTE_DICOM ||
        this.studiesSourceType == SourceType.REMOTE_DICOM_WEB
      );
    },
    sameLocalStudyLink() {
      return (
        "/filtered-studies?StudyInstanceUID=" +
        this.studyMainDicomTagsLocalCopy.StudyInstanceUID
      );
    },
  },
  watch: {
    allLabels(newValue, oldValue) {
      this.labelsComponentKey++;
    },
  },
  components: {
    SeriesItem,
    SeriesList,
    ResourceButtonGroup,
    ResourceDetailText,
    LabelsEditor,
    AuditLogs,
  },
  methods: {
    onDeletedStudy() {
      this.$emit("deletedStudy", this.studyId);
    },
    async reloadSeriesList() {
      if (this.studiesSourceType == SourceType.LOCAL_ORTHANC) {
        this.studySeries = await api.getStudySeries(this.studyId);
      } else if (this.studiesSourceType == SourceType.REMOTE_DICOM) {
        let remoteSeries = await api.remoteDicomFind(
          "Series",
          this.studiesRemoteSource,
          {
            StudyInstanceUID: this.studyMainDicomTagsLocalCopy.StudyInstanceUID,
            PatientID: this.patientMainDicomTags.PatientID,
            NumberOfSeriesRelatedInstances: "",
            Modality: "",
            SeriesDescription: "",
            SeriesNumber: "",
          },
          false,
        );
        this.studySeries = remoteSeries.map((s) => {
          return {
            ID: s["SeriesInstanceUID"],
            MainDicomTags: s,
          };
        });
      } else if (this.studiesSourceType == SourceType.REMOTE_DICOM_WEB) {
        let remoteSeries = await api.qidoRs(
          "Series",
          this.studiesRemoteSource,
          {
            StudyInstanceUID: this.studyMainDicomTagsLocalCopy.StudyInstanceUID,
            NumberOfSeriesRelatedInstances: "",
            Modality: "",
            SeriesDescription: "",
            SeriesNumber: "",
            SeriesDate: "",
            SeriesTime: "",
            BodyPartExamined: "",
            ProtocolName: "",
          },
          false,
        );
        this.studySeries = remoteSeries.map((s) => {
          return {
            ID: s["SeriesInstanceUID"],
            MainDicomTags: s,
          };
        });
      }
    },
  },
};
</script>

<template>
  <div class="study-details-container">
    <!-- Top Row: Labels + Action Buttons -->
    <div class="top-row">
      <div class="actions-buttons d-flex">
        <ResourceButtonGroup :resourceOrthancId="this.studyId" :resourceLevel="'study'"
          :patientMainDicomTags="this.patientMainDicomTags" :studyMainDicomTags="this.studyMainDicomTagsLocalCopy"
          :resourceDicomUid="this.studyMainDicomTagsLocalCopy.StudyInstanceUID" :studySeries="this.studySeries"
          @deletedResource="onDeletedStudy">
        </ResourceButtonGroup>
      </div>
      <div class="labels-section" v-if="showLabels && uiOptions.EnableEditLabels">
        <LabelsEditor :labels="labels" :title="'labels.study_details_title'" :key="labelsComponentKey"
          :studyId="studyId"></LabelsEditor>
      </div>
      <div class="labels-section" v-else-if="showLabels && !uiOptions.EnableEditLabels">
        {{ $t("labels.study_details_title") }}
        <span v-for="label in labels" :key="label" class="label badge">{{
          label
        }}</span>
      </div>
      <div class="labels-section labels-placeholder" v-else></div>
    </div>

    <!-- Main Content: Two Cards -->
    <div class="details-content row flex-xl-row-reverse flex-lg-row">
      <!-- Study Information Card -->
      <div class="info-card col-xl-3 col-lg-6 col-md-12">
        <div class="card-header">
          <span class="card-title">STUDY INFORMATION</span>
        </div>
        <div class="card-body">
          <div class="field-list">
            <ResourceDetailText v-for="tag in uiOptions.StudyMainTags" :key="tag" :tags="studyMainDicomTagsLocalCopy"
              :tag="tag"></ResourceDetailText>
          </div>
        </div>
      </div>

      <!-- Patient Information Card -->
      <div class="info-card col-xl-3 col-lg-6 col-md-12">
        <div class="card-header">
          <span class="card-title">PATIENT INFORMATION</span>
        </div>
        <div class="card-body">
          <div class="field-list">
            <ResourceDetailText v-for="tag in uiOptions.PatientMainTags" :key="tag" :tags="patientMainDicomTags"
              :tag="tag"></ResourceDetailText>
          </div>

          <!-- Other Studies Info Messages -->
          <div v-if="
            isLocalOrthanc &&
            hasLoadedSamePatientsStudiesCount &&
            samePatientStudiesCount > 1
          " class="info-message info-message--link">
            <i class="fa fa-folder-open info-icon"></i>
            <span>
              {{
                $t("this_patient_has_other_studies", {
                  count: samePatientStudiesCount,
                })
              }}.
              <router-link :to="samePatientStudiesLink">
                {{ $t("this_patient_has_other_studies_show") }}
              </router-link>
            </span>
          </div>

          <div v-if="
            isLocalOrthanc &&
            hasLoadedSamePatientsStudiesCount &&
            samePatientStudiesCount == 1
          " class="info-message info-message--success">
            <i class="fa fa-circle-check info-icon"></i>
            <span>{{ $t("this_patient_has_no_other_studies") }}</span>
          </div>

          <div v-if="
            isRemoteSource &&
            hasLoadedSamePatientsStudiesCount &&
            samePatientStudiesCount > 1
          " class="info-message info-message--link">
            <i class="fa fa-folder-open info-icon"></i>
            <span>
              {{
                $t("this_remote_patient_has_local_studies", {
                  count: samePatientStudiesCount,
                })
              }}
              <router-link :to="samePatientStudiesLink">
                {{ $t("this_patient_has_other_studies_show") }}
              </router-link>
            </span>
          </div>

          <div v-if="isRemoteSource && remoteStudyFoundLocally" class="info-message info-message--link">
            <i class="fa fa-database info-icon"></i>
            <span>
              {{ $t("this_study_is_already_stored_locally") }}
              <router-link :to="sameLocalStudyLink">
                {{ $t("this_study_is_already_stored_locally_show") }}
              </router-link>
            </span>
          </div>
        </div>
      </div>
      <!-- Series List -->
      <div class="series-section col-xl-6 col-md-12">
        <SeriesList :studyId="this.studyId" :studyMainDicomTags="this.studyMainDicomTagsLocalCopy"
          :patientMainDicomTags="this.patientMainDicomTags" :studySeries="this.studySeries"
          @deletedStudy="onDeletedStudy"></SeriesList>
      </div>
    </div>

    <!-- Audit Logs Link -->
    <div v-if="uiOptions.EnableAuditLogs" class="audit-logs-row">
      <router-link class="audit-link" :to="'/audit-logs?resource-id=' + this.studyId">
        <i class="fa fa-clipboard-list"></i>
        {{ $t("audit_logs.expand_logs") }}
      </router-link>
    </div>
  </div>
</template>

<style scoped>
/* =============================================================================
   STUDYDETAILS.VUE - CARD-BASED LAYOUT (D-MIS Design System)
   ============================================================================= */

.study-details-container {
  margin: 0.5rem 1%;
  font-family:
    "Segoe UI",
    system-ui,
    -apple-system,
    sans-serif;
}

/* Top Row: Labels + Buttons */
.top-row {
  display: flex;
  align-items: center;
  gap: 12px;
  margin-bottom: 10px;
}

/* Labels Section */
.labels-section {
  flex: 1;
  background: #f8fafc;

  border-radius: 6px;

  min-height: 36px;
  display: flex;
  align-items: center;
  flex-wrap: wrap;
  gap: 4px;
}

.labels-section.labels-placeholder {
  background: transparent;
  border: 1px dashed #e2e8f0;
}

/* Action Buttons in top row */
.actions-buttons {
  flex-shrink: 0;
}

.label.badge {
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

.label.badge:hover {
  transform: translateY(-2px);
  box-shadow: 0 4px 8px rgba(59, 130, 246, 0.3);
}

/* Main Content Layout */
.details-content {
  display: flex;
  gap: 12px;
  margin-bottom: 10px;
}

/* Info Cards */
.info-card {
  flex: 1;
  background: #ffffff;
  border: 1px solid #e2e8f0;
  border-radius: 6px;
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.05);
  overflow: hidden;
}

.card-header {
  padding: 8px 12px;
  border-bottom: 1px solid #e2e8f0;
  background: #ffffff;
}

.card-title {
  color: #2563eb;
  font-size: 11px;
  font-weight: 600;
  letter-spacing: 0.5px;
  text-transform: uppercase;
}

.card-body {
  padding: 8px 12px;
}

/* Field List */
.field-list {
  display: flex;
  flex-direction: column;
  gap: 0;
}

.field-list :deep(li) {
  display: flex;
  flex-direction: column !important;
  align-items: flex-start !important;
  padding: 8px 0;
  border-bottom: 1px solid #f1f5f9;
  list-style: none;
}

.field-list :deep(li:last-child) {
  border-bottom: none;
}

/* Label styling - название тега сверху */
.field-list :deep(.details-label) {
  color: #64748b;
  font-size: 9px !important;
  font-weight: 500 !important;
  text-transform: uppercase;
  letter-spacing: 0.5px;
  margin-bottom: 2px;
  margin-left: 0 !important;
  max-width: 100% !important;
  order: 1;
}

/* Убираем двоеточие из лейбла через CSS невозможно, но уменьшаем его визуальное влияние */

/* Value styling - значение тега снизу */
.field-list :deep(.details) {
  color: #1e293b;
  font-size: 13px !important;
  font-weight: 400 !important;
  word-break: break-word;
  margin-left: 0 !important;
  max-width: 100% !important;
  text-align: left;
  order: 2;
}

.field-list :deep(.details:empty)::after {
  content: "-";
  color: #94a3b8;
}

/* Copy button */
.field-list :deep(.copy-button),
.field-list :deep(button) {
  order: 3;
  margin-top: 2px;
  opacity: 0.5;
  transform: scale(0.8);
}

.field-list :deep(li:hover) .copy-button,
.field-list :deep(li:hover) button {
  opacity: 1;
}

/* Info Messages */
.info-message {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-top: 10px;
  padding: 8px 10px;
  border-radius: 4px;
  font-size: 12px;
  line-height: 1.4;
}

.info-message--success {
  background: linear-gradient(135deg, #ecfdf5 0%, #d1fae5 100%);
  border: 1px solid #a7f3d0;
  color: #065f46;
}

.info-message--success .info-icon {
  color: #10b981;
}

.info-message--link {
  background: linear-gradient(135deg, #eff6ff 0%, #dbeafe 100%);
  border: 1px solid #bfdbfe;
  color: #1e40af;
}

.info-message--link .info-icon {
  color: #3b82f6;
}

.info-icon {
  font-size: 14px;
  flex-shrink: 0;
}

.info-message a,
.info-message .router-link {
  color: #2563eb;
  font-weight: 600;
  text-decoration: none;
  transition: color 0.2s ease;
}

.info-message a:hover,
.info-message .router-link:hover {
  color: #1d4ed8;
  text-decoration: underline;
}

/* Audit Logs Row */
.audit-logs-row {
  background: linear-gradient(135deg, #fef3c7 0%, #fde68a 100%);
  border: 1px solid #fcd34d;
  border-radius: 6px;
  padding: 6px 12px;
  margin-bottom: 10px;
}

.audit-link {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  color: #92400e;
  font-weight: 600;
  font-size: 12px;
  text-decoration: none;
  transition: all 0.2s ease;
}

.audit-link:hover {
  color: #78350f;
  transform: translateX(4px);
}

.audit-link i {
  font-size: 14px;
}

/* Series Section */
.series-section {
  background: linear-gradient(180deg, #fafbfc 0%, #f1f5f9 100%);
  border: 1px solid #e2e8f0;
  border-radius: 6px;
  padding: 10px;
}

/* Responsive Design */
@media (max-width: 1200px) {
  .details-content {
    gap: 12px;
  }

  .info-card {
    min-width: 0;
  }
}

@media (max-width: 992px) {
  .details-content {
    flex-wrap: wrap;
  }

  .info-card {
    flex: 1 1 calc(50% - 6px);
    min-width: 280px;
  }
}

@media (max-width: 768px) {
  .study-details-container {
    margin: 0.25rem 0.5%;
  }

  /* Top row becomes 2 rows on mobile */
  .top-row {
    flex-direction: column;
    gap: 8px;
  }

  .labels-section {
    width: 100%;
  }

  .actions-buttons {
    width: 100%;
    display: flex;
    justify-content: center;
  }

  .details-content {
    flex-direction: column;
    gap: 8px;
  }

  .info-card {
    flex: 1 1 100%;
  }

  .card-body {
    padding: 8px;
  }

  .field-list :deep(.details-label) {
    font-size: 8px;
  }

  .field-list :deep(.details) {
    font-size: 12px;
  }
}

/* Accessibility */
.info-card:focus-within {
  outline: 2px solid #3b82f6;
  outline-offset: 2px;
}

@media (prefers-reduced-motion: reduce) {

  .label.badge,
  .info-message a,
  .audit-link {
    transition: none;
  }
}

/* Print Styles */
@media print {
  .study-details-container {
    margin: 0;
  }

  .top-row,
  .audit-logs-row {
    display: none;
  }

  .info-card {
    box-shadow: none;
    border: 1px solid #000;
    break-inside: avoid;
  }

  .details-content {
    flex-direction: column;
  }
}
</style>

<style>
/* =============================================================================
   GLOBAL STYLES (not scoped) - for child components
   ============================================================================= */

/* ResourceButtonGroup Container */
.resource-button-group,
.study-button-group {
  display: flex;
  flex-direction: row;
  flex-wrap: wrap;
  gap: 4px;
  align-items: center;
  justify-content: flex-end;
  padding: 0;
}

.resource-button-group.horizontal,
.study-button-group.horizontal {
  flex-direction: row;
  flex-wrap: wrap;
  justify-content: flex-end;
  align-items: center;
}

/* Base Button Styles - D-MIS Gradient */
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
  transition: all 0.25s ease;
  user-select: none;
  width: 32px;
  height: 32px;
  min-width: 32px;
  padding: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  background: linear-gradient(135deg, #3b82f6 0%, #8b5cf6 100%);
  border: none;
  border-radius: 6px;
  color: white;
  font-size: 14px;
  box-shadow: 0 2px 6px rgba(59, 130, 246, 0.25);
  position: relative;
}

.resource-button-group button:hover,
.resource-button-group a:hover,
.resource-button-group .btn:hover {
  background: linear-gradient(135deg, #2563eb 0%, #7c3aed 100%);
  box-shadow: 0 4px 16px rgba(139, 92, 246, 0.4);
  transform: translateY(-2px) scale(1.05);
}

.resource-button-group button:active,
.resource-button-group a:active,
.resource-button-group .btn:active {
  transform: translateY(0) scale(1.02);
  box-shadow: 0 2px 6px rgba(59, 130, 246, 0.3);
}

.resource-button-group button i,
.resource-button-group a i,
.resource-button-group .btn i {
  font-size: 13px;
  color: white;
  line-height: 1;
  transition: none;
}

/* Color Variants - D-MIS Style */

/* Primary/View (Blue-Purple D-MIS gradient) */
.resource-button-group .btn-primary,
.resource-button-group button[data-action="view"],
.resource-button-group a[href*="viewer"] {
  background: linear-gradient(135deg, #3b82f6 0%, #8b5cf6 100%);
}

.resource-button-group .btn-primary:hover,
.resource-button-group button[data-action="view"]:hover,
.resource-button-group a[href*="viewer"]:hover {
  background: linear-gradient(135deg, #2563eb 0%, #7c3aed 100%);
  box-shadow: 0 4px 16px rgba(139, 92, 246, 0.45);
}

/* Download (Purple-Pink) */
.resource-button-group .btn-download,
.resource-button-group button[data-action="download"] {
  background: linear-gradient(135deg, #8b5cf6 0%, #ec4899 100%);
}

.resource-button-group .btn-download:hover,
.resource-button-group button[data-action="download"]:hover {
  background: linear-gradient(135deg, #7c3aed 0%, #db2777 100%);
  box-shadow: 0 4px 16px rgba(236, 72, 153, 0.4);
}

/* Send (Cyan-Blue) */
.resource-button-group .btn-send,
.resource-button-group button[data-action="send"] {
  background: linear-gradient(135deg, #06b6d4 0%, #3b82f6 100%);
}

.resource-button-group .btn-send:hover,
.resource-button-group button[data-action="send"]:hover {
  background: linear-gradient(135deg, #0891b2 0%, #2563eb 100%);
  box-shadow: 0 4px 16px rgba(6, 182, 212, 0.4);
}

/* Edit (Orange-Yellow) */
.resource-button-group .btn-edit,
.resource-button-group button[data-action="edit"] {
  background: linear-gradient(135deg, #f59e0b 0%, #eab308 100%);
}

.resource-button-group .btn-edit:hover,
.resource-button-group button[data-action="edit"]:hover {
  background: linear-gradient(135deg, #d97706 0%, #ca8a04 100%);
  box-shadow: 0 4px 16px rgba(245, 158, 11, 0.4);
}

/* Delete (Red-Pink) */
.resource-button-group .btn-danger,
.resource-button-group .btn-delete,
.resource-button-group button[data-action="delete"] {
  background: linear-gradient(135deg, #ef4444 0%, #ec4899 100%);
}

.resource-button-group .btn-danger:hover,
.resource-button-group .btn-delete:hover,
.resource-button-group button[data-action="delete"]:hover {
  background: linear-gradient(135deg, #dc2626 0%, #db2777 100%);
  box-shadow: 0 4px 16px rgba(239, 68, 68, 0.4);
}

/* Success (Green-Teal) */
.resource-button-group .btn-success,
.resource-button-group button[data-action="approve"] {
  background: linear-gradient(135deg, #10b981 0%, #14b8a6 100%);
}

.resource-button-group .btn-success:hover,
.resource-button-group button[data-action="approve"]:hover {
  background: linear-gradient(135deg, #059669 0%, #0d9488 100%);
  box-shadow: 0 4px 16px rgba(16, 185, 129, 0.4);
}

/* Button States */
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

.resource-button-group button.active,
.resource-button-group .btn.active {
  background: linear-gradient(135deg, #3b82f6 0%, #2563eb 100%);
  box-shadow: 0 4px 12px rgba(59, 130, 246, 0.4);
  transform: scale(1.05);
}

/* Tooltips */
.resource-button-group button[title],
.resource-button-group .btn[title] {
  position: relative;
}

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

/* Button Sizes */
.resource-button-group.small button,
.resource-button-group.small .btn,
.resource-button-group .btn-sm {
  width: 32px;
  height: 32px;
  min-width: 32px;
  font-size: 13px;
  border-radius: 8px;
}

.resource-button-group.small button i,
.resource-button-group .btn-sm i {
  font-size: 13px;
}

.resource-button-group.large button,
.resource-button-group.large .btn,
.resource-button-group .btn-lg {
  width: 46px;
  height: 46px;
  min-width: 46px;
  font-size: 20px;
  border-radius: 12px;
}

.form-control.dropdown.form-select {
  cursor: pointer;
  position: relative !important;
  width: auto;
  height: 48px !important;
  display: flex;
}

ul#tags-menu-1 {
  position: relative !important;
  top: 0;
  left: 0;
}

.resource-button-group.large button i,
.resource-button-group .btn-lg i {
  font-size: 20px;
}

.btn-icon {
  background-color: var(--nav-side-bg-color);
}

/* Dropdown */
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

/* Responsive (Global) */
@media (max-width: 768px) {
  .resource-button-group {
    flex-direction: row;
    justify-content: center;
    gap: 6px;
  }

  .resource-button-group button,
  .resource-button-group .btn {
    width: 34px;
    height: 34px;
    min-width: 34px;
    font-size: 14px;
  }
}

/* Accessibility */
.resource-button-group button:focus-visible,
.resource-button-group .btn:focus-visible {
  outline: 2px solid #3b82f6;
  outline-offset: 2px;
}

@media (prefers-contrast: high) {

  .resource-button-group button,
  .resource-button-group .btn {
    border: 2px solid rgba(255, 255, 255, 0.3);
  }
}

@media (prefers-reduced-motion: reduce) {

  .resource-button-group button,
  .resource-button-group .btn {
    transition: none;
    animation: none;
  }
}

/* Print */
@media print {
  .resource-button-group {
    display: none !important;
  }
}
</style>
