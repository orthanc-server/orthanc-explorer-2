<script>
import ResourceButtonGroup from "./ResourceButtonGroup.vue";
import InstanceList from "./InstanceList.vue";
import InstanceListExtended from "./InstanceListExtended.vue";
import ResourceDetailText from "./ResourceDetailText.vue";
import { mapState, mapGetters } from "vuex";
import api from "../orthancApi";
import SourceType from "../helpers/source-type";

export default {
  props: [
    "seriesId",
    "seriesMainDicomTags",
    "studyMainDicomTags",
    "patientMainDicomTags",
    "instancesIds",
  ],
  emits: ["deletedSeries"],
  setup() { },
  data() {
    return {
      seriesInstances: [],
    };
  },
  computed: {
    ...mapState({
      uiOptions: (state) => state.configuration.uiOptions,
      studiesSourceType: (state) => state.studies.sourceType,
      studiesRemoteSource: (state) => state.studies.remoteSource,
      hasExtendedFind: (state) => state.configuration.hasExtendedFind,
    }),
    useExtendedInstanceList() {
      return (
        this.hasExtendedFind &&
        this.studiesSourceType == SourceType.LOCAL_ORTHANC
      );
    },
  },
  async mounted() {
    if (this.studiesSourceType == SourceType.LOCAL_ORTHANC) {
      if (this.useExtendedInstanceList) {
        this.seriesInstances = await api.getSeriesInstancesExtended(
          this.seriesId,
          null,
        );
      } else {
        this.seriesInstances = await api.getSeriesInstances(this.seriesId);
      }
    } else if (this.studiesSourceType == SourceType.REMOTE_DICOM) {
      let remoteInstances = await api.remoteDicomFind(
        "Instance",
        this.studiesRemoteSource,
        {
          StudyInstanceUID: this.studyMainDicomTags.StudyInstanceUID,
          PatientID: this.patientMainDicomTags.PatientID,
          SeriesInstanceUID: this.seriesMainDicomTags.SeriesInstanceUID,
          SOPInstanceUID: "",
          InstanceNumber: "",
          NumberOfFrames: "",
        },
        false /* isUnique */,
      );
      this.seriesInstances = remoteInstances.map((s) => {
        return {
          ID: s["SOPInstanceUID"],
          MainDicomTags: s,
        };
      });
      this.seriesInstances = this.seriesInstances.sort((a, b) =>
        (parseInt(a.MainDicomTags.InstanceNumber) ??
          a.MainDicomTags.SOPInstanceUID) <
          (parseInt(b.MainDicomTags.InstanceNumber) ??
            b.MainDicomTags.SOPInstanceUID)
          ? 1
          : -1,
      );
    } else if (this.studiesSourceType == SourceType.REMOTE_DICOM_WEB) {
      let remoteInstances = await api.qidoRs(
        "Instance",
        this.studiesRemoteSource,
        {
          StudyInstanceUID: this.studyMainDicomTags.StudyInstanceUID,
          SeriesInstanceUID: this.seriesMainDicomTags.SeriesInstanceUID,
          SOPInstanceUID: "",
          InstanceNumber: "",
          NumberOfFrames: "",
        },
        false /* isUnique */,
      );
      this.seriesInstances = remoteInstances.map((s) => {
        return {
          ID: s["SOPInstanceUID"],
          MainDicomTags: s,
        };
      });
      this.seriesInstances = this.seriesInstances.sort((a, b) =>
        (parseInt(a.MainDicomTags.InstanceNumber) ??
          a.MainDicomTags.SOPInstanceUID) <
          (parseInt(b.MainDicomTags.InstanceNumber) ??
            b.MainDicomTags.SOPInstanceUID)
          ? 1
          : -1,
      );
    }
  },
  components: {
    ResourceButtonGroup,
    InstanceList,
    InstanceListExtended,
    ResourceDetailText,
  },
  methods: {
    onDeletedInstance(instanceId) {
      const pos = this.instancesIds.indexOf(instanceId);
      if (pos >= 0) {
        this.instancesIds.splice(pos, 1);
        if (this.instancesIds.length == 0) {
          this.$emit("deletedSeries", this.seriesId);
        }
      }
    },
    onDeletedSeries() {
      this.$emit("deletedSeries", this.seriesId);
    },
  },
};
</script>

<template>
  <div class="series-details-table">
    <div class="row g-3 mb-3">
      <div class="col-12 col-md-4 series-button-group d-flex flex-wrap">
        <ResourceButtonGroup :resourceOrthancId="this.seriesId" :resourceLevel="'series'"
          :resourceDicomUid="this.seriesMainDicomTags.SeriesInstanceUID" :studyMainDicomTags="this.studyMainDicomTags"
          :seriesMainDicomTags="this.seriesMainDicomTags" :patientMainDicomTags="this.patientMainDicomTags"
          :seriesInstances="this.seriesInstances" :customClass="'instance-button-group'"
          @deletedResource="onDeletedSeries"></ResourceButtonGroup>
      </div>

      <div class="col-12 col-md-8 d-flex row">
        <ResourceDetailText v-for="tag in uiOptions.SeriesMainTags" :key="tag" :tags="seriesMainDicomTags" :tag="tag"
          :showIfEmpty="true"></ResourceDetailText>
      </div>
    </div>

    <div class="row">
      <div class="col-12">
        <InstanceList v-if="!useExtendedInstanceList" :seriesId="this.seriesId"
          :seriesMainDicomTags="this.seriesMainDicomTags" :patientMainDicomTags="this.patientMainDicomTags"
          :studyMainDicomTags="this.studyMainDicomTags" :instancesIds="this.instancesIds"
          :seriesInstances="this.seriesInstances" @deletedInstance="onDeletedInstance"></InstanceList>
        <InstanceListExtended v-if="useExtendedInstanceList" :seriesId="this.seriesId"
          :seriesMainDicomTags="this.seriesMainDicomTags" :patientMainDicomTags="this.patientMainDicomTags"
          :studyMainDicomTags="this.studyMainDicomTags" :instancesIds="this.instancesIds"
          :seriesInstances="this.seriesInstances" @deletedInstance="onDeletedInstance"></InstanceListExtended>
      </div>
    </div>
  </div>
</template>

<style>
.series-details-table {
  font-size: 0.875rem;
}

.cut-text {
  text-overflow: ellipsis;
  overflow: hidden;
  white-space: pre;
  /* preserve double white spaces (https://github.com/orthanc-server/orthanc-explorer-2/issues/30) */
  max-width: 100% !important;
}

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
<style>
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

.study-details-expanded> :first-child {
  border-left: 4px solid #3b82f6;
}

.study-table td {
  border-bottom: none !important;
}


/* =============================================================================
TAGS
   ============================================================================= */
li.tagLabel.active {
  text-overflow: ellipsis;
  overflow: hidden;
  white-space: pre;
  /* preserve double white spaces (https://github.com/orthanc-server/orthanc-explorer-2/issues/30) */
  max-width: 100%;
}
</style>
