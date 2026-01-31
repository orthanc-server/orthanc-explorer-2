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
  emits: ["deletedStudy"],
  data() {
    return {
      study: {},
      loaded: false,
      expanded: false,
      collapseElement: null,
      selected: false,
      pdfReports: [],
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
  },
  methods: {
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
  },
  computed: {
    ...mapState({
      uiOptions: (state) => state.configuration.uiOptions,
      studies: (state) => state.studies.studies,
      studiesSourceType: (state) => state.studies.sourceType,
      selectedStudiesIds: (state) => state.studies.selectedStudiesIds,
      allLabels: (state) => state.labels.allLabels,
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
      return dateHelpers.formatDateForDisplay(
        this.study.PatientMainDicomTags.PatientBirthDate,
        this.uiOptions.DateFormat,
      );
    },
    formattedStudyDate() {
      return dateHelpers.formatDateForDisplay(
        this.study.MainDicomTags.StudyDate,
        this.uiOptions.DateFormat,
      );
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
      let span = this.uiOptions.StudyListColumns.length + 1; // +1 for the 'checkbox'
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
    <tr
      v-if="loaded"
      :class="{
        'study-row-collapsed': !expanded,
        'study-row-expanded': expanded,
        'study-row-show-labels': showLabels,
      }"
    >
      <td>
        <div class="form-check">
          <input
            class="form-check-input"
            type="checkbox"
            v-model="selected"
            @click="clickedSelect"
          />
        </div>
      </td>
      <td v-if="hasPrimaryViewerIcon" class="td-viewer-icon">
        <TokenLinkButton
          v-if="primaryViewerUrl"
          level="study"
          :linkUrl="primaryViewerUrl"
          :resourcesOrthancId="[study.ID]"
          linkType="icon"
          iconClass="bi bi-eye-fill"
          :tokenType="primaryViewerTokenType"
          :opensInNewTab="true"
        >
        </TokenLinkButton>
      </td>
      <td v-if="hasPrimaryViewerIconPlaceholder"></td>
      <td v-if="hasPdfReportIcon" class="td-pdf-icon">
        <TokenLinkButton
          v-for="pdfReport in pdfReports"
          :key="pdfReport.id"
          level="study"
          :linkUrl="pdfReport.url"
          :resourcesOrthancId="[study.ID]"
          linkType="icon"
          iconClass="bi bi-file-earmark-text"
          :tokenType="'download-instant-link'"
          :opensInNewTab="true"
          :title="pdfReport.title"
        >
        </TokenLinkButton>
      </td>
      <td v-if="hasPdfReportIconPlaceholder"></td>

      <td
        v-for="columnTag in uiOptions.StudyListColumns"
        :key="columnTag"
        class="cut-text"
        :class="{
          'text-center':
            columnTag in
            [
              'modalities',
              'seriesCount',
              'instancesCount',
              'seriesAndInstancesCount',
            ],
        }"
        data-bs-toggle="collapse"
        v-bind:data-bs-target="'#study-details-' + this.studyId"
      >
        <span
          v-if="columnTag == 'StudyDate'"
          data-bs-toggle="tooltip"
          v-bind:title="formattedStudyDate"
          >{{ formattedStudyDate }}
        </span>
        <span
          v-else-if="columnTag == 'AccessionNumber'"
          data-bs-toggle="tooltip"
          v-bind:title="study.MainDicomTags.AccessionNumber"
          >{{ study.MainDicomTags.AccessionNumber }}
        </span>
        <span
          v-else-if="columnTag == 'PatientID'"
          data-bs-toggle="tooltip"
          v-bind:title="study.PatientMainDicomTags.PatientID"
          >{{ study.PatientMainDicomTags.PatientID }}
        </span>
        <span
          v-else-if="columnTag == 'PatientSex'"
          data-bs-toggle="tooltip"
          v-bind:title="study.PatientMainDicomTags.PatientSex"
          >{{ study.PatientMainDicomTags.PatientSex }}
        </span>
        <span
          v-else-if="columnTag == 'PatientName'"
          data-bs-toggle="tooltip"
          v-bind:title="formattedPatientName"
          >{{ formattedPatientName }}
        </span>
        <span
          v-else-if="columnTag == 'PatientBirthDate'"
          data-bs-toggle="tooltip"
          v-bind:title="formattedPatientBirthDate"
          >{{ formattedPatientBirthDate }}
        </span>
        <span
          v-else-if="columnTag == 'StudyDescription'"
          data-bs-toggle="tooltip"
          v-bind:title="study.MainDicomTags.StudyDescription"
          >{{ study.MainDicomTags.StudyDescription }}
        </span>
        <span
          v-else-if="columnTag == 'modalities'"
          data-bs-toggle="tooltip"
          v-bind:title="modalitiesInStudyForDisplay"
          >{{ modalitiesInStudyForDisplay }}
        </span>
        <span
          v-else-if="columnTag == 'seriesCount'"
          data-bs-toggle="tooltip"
          v-bind:title="seriesCount"
          >{{ seriesCount }}
        </span>
        <span
          v-else-if="columnTag == 'instancesCount'"
          data-bs-toggle="tooltip"
          v-bind:title="instancesCount"
          >{{ instancesCount }}
        </span>
        <span
          v-else-if="columnTag == 'seriesAndInstancesCount'"
          data-bs-toggle="tooltip"
          v-bind:title="seriesAndInstancesCount"
          >{{ seriesAndInstancesCount }}
        </span>
        <span v-else>{{ study.MainDicomTags[columnTag] }} </span>
      </td>
    </tr>
    <tr v-show="showLabels">
      <td></td>
      <td v-if="hasPrimaryViewerColumn"></td>
      <td v-if="hasPdfReportIconColumn"></td>
      <td :colspan="uiOptions.StudyListColumns.length" class="label-row">
        <span v-for="label in study.Labels" :key="label" class="label badge">{{
          label
        }}</span>
        <span v-if="!hasLabels">&nbsp;</span>
      </td>
    </tr>
    <tr
      v-show="loaded"
      class="collapse"
      :class="{ 'study-details-expanded': expanded }"
      v-bind:id="'study-details-' + this.studyId"
      ref="study-collapsible-details"
    >
      <td v-if="loaded && expanded" :colspan="colSpanStudyDetails">
        <StudyDetails
          :studyId="this.studyId"
          :studyMainDicomTags="this.study.MainDicomTags"
          :patientMainDicomTags="this.study.PatientMainDicomTags"
          :labels="this.study.Labels"
          @deletedStudy="onDeletedStudy"
        ></StudyDetails>
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
  background: linear-gradient(
    90deg,
    rgba(59, 130, 246, 0.08) 0%,
    rgba(59, 130, 246, 0.03) 100%
  );
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

.study-row-expanded > td {
  padding: 14px 10px !important;
  color: #1e293b;
}

.study-row-expanded > :first-child {
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
}

.study-details-expanded:hover {
  background: linear-gradient(180deg, #f1f5f9 0%, #e2e8f0 100%) !important;
}

/* =============================================================================
   LABELS ROW
   ============================================================================= */
.label-row {
  border-top: none !important;
  border-bottom: none !important;
  padding: 6px 10px !important;
  background: linear-gradient(
    90deg,
    rgba(59, 130, 246, 0.03) 0%,
    transparent 100%
  );
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
  content: " a0"; /* non-breaking space */
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
  max-width: 0; /* Force text truncation with ellipsis */
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

/* Viewer Icon Hover */
.td-viewer-icon a:hover {
  background: linear-gradient(
    135deg,
    rgba(59, 130, 246, 0.1) 0%,
    rgba(59, 130, 246, 0.05) 100%
  );
}

.td-viewer-icon a:hover .bi {
  color: #3b82f6;
  transform: scale(1.2);
}

/* PDF Icon Hover */
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
}

/* Icon Active State */
.td-viewer-icon a:active .bi,
.td-pdf-icon a:active .bi {
  transform: scale(1.1);
}

/* Multiple PDF Reports */
.td-pdf-icon a + a {
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
  background: linear-gradient(
    90deg,
    rgba(59, 130, 246, 0.05) 0%,
    transparent 100%
  );
  border-left: 3px solid #3b82f6;
}

.study-row-collapsed:has(.form-check-input:checked):hover {
  background: linear-gradient(
    90deg,
    rgba(59, 130, 246, 0.1) 0%,
    rgba(59, 130, 246, 0.05) 100%
  );
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

  .td-viewer-icon,
  .td-pdf-icon {
    display: none;
  }
}
</style>
