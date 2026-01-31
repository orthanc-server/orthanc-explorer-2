<script>
import api from "../orthancApi";
import ResourceButtonGroup from "./ResourceButtonGroup.vue";
import TagsTree from "./TagsTree.vue";
import { mapState, mapGetters } from "vuex";
import SourceType from "../helpers/source-type";

export default {
  props: [
    "instanceId",
    "seriesMainDicomTags",
    "studyMainDicomTags",
    "patientMainDicomTags",
  ],
  emits: ["deletedInstance"],
  data() {
    return {
      tags: {},
      headers: {},
      loaded: false,
    };
  },
  computed: {
    ...mapState({
      uiOptions: (state) => state.configuration.uiOptions,
      studiesSourceType: (state) => state.studies.sourceType,
    }),
  },
  async mounted() {
    if (this.studiesSourceType == SourceType.LOCAL_ORTHANC) {
      this.tags = await api.getInstanceTags(this.instanceId);
      this.headers = await api.getInstanceHeader(this.instanceId);
    } else if (
      this.studiesSourceType == SourceType.REMOTE_DICOM ||
      this.studiesSourceType == SourceType.REMOTE_DICOM_WEB
    ) {
      this.tags = {
        "0008,0018": {
          Name: "SOPInstanceUID",
          Type: "String",
          Value: this.instanceId,
        },
      };
    }
    this.loaded = true;
  },
  components: { ResourceButtonGroup, TagsTree },
  methods: {
    onDeletedInstance() {
      this.$emit("deletedInstance", this.instanceId);
    },
  },
};
</script>

<template>
  <div v-if="loaded" class="instance-details-container">
    <!-- Верхняя секция: Headers и кнопки -->
    <div class="d-flex flex-column flex-lg-row gap-3 mb-3">
      <div class="flex-grow-1" style="flex-basis: 80%">
        <div class="info-section">
          <TagsTree :tags="headers"></TagsTree>
        </div>
      </div>
      <div class="button-section" style="flex-basis: 20%">
        <ResourceButtonGroup
          :resourceOrthancId="this.instanceId"
          :resourceLevel="'instance'"
          :studyMainDicomTags="this.studyMainDicomTags"
          :seriesMainDicomTags="this.seriesMainDicomTags"
          :patientMainDicomTags="this.patientMainDicomTags"
          :instanceTags="this.tags"
          :instanceHeaders="this.headers"
          @deletedResource="onDeletedInstance"
        ></ResourceButtonGroup>
      </div>
    </div>

    <!-- Нижняя секция: Tags -->
    <div class="info-section">
      <TagsTree :tags="tags"></TagsTree>
    </div>
  </div>
</template>

<style scoped>
.instance-details-table {
  margin-top: var(--details-top-margin);
  margin-left: 5%;
  width: 95% !important;
  background-color: var(--instance-details-bg-color) !important;
  font-size: 0.8rem;
}

.instance-details-table td {
  vertical-align: top;
}

.instance-details-table > :not(caption) > * > * {
  background-color: var(--instance-details-bg-color) !important;
}

.instance-details-table > * > * {
  background-color: var(--instance-details-bg-color) !important;
}

.details-label {
  font-weight: 700;
  max-width: 30%;
  overflow: hidden;
  text-overflow: ellipsis;
}

.details {
  margin-left: auto !important;
  font-weight: 500;
  max-width: 25vw;
  overflow: hidden;
  text-overflow: ellipsis;
}
</style>
<style>
.instance-button-group i {
  font-size: 1.2rem !important;
}
.instance-details-container {
  font-size: 0.875rem;
}

.info-section {
  background: #f8f9fa;
  border: 1px solid #dee2e6;
  border-radius: 0.25rem;
  padding: 1.25rem;
  overflow: hidden;
}

.button-section {
  display: flex;
  flex-direction: column;
  align-items: stretch;
  gap: 0.5rem;
}

/* Адаптация под мобильные */
@media (max-width: 991.98px) {
  .button-section {
    flex-direction: row;
    flex-wrap: wrap;
  }
}

/* Стили для отображения полей в формате "название сверху - значение снизу" */
.info-section :deep(.tag-item) {
  display: flex;
  flex-direction: column;
  margin-bottom: 1rem;
  padding-bottom: 0.75rem;
  border-bottom: 1px solid #e9ecef;
}

.info-section :deep(.tag-label) {
  font-size: 0.75rem;
  font-weight: 600;
  color: #6c757d;
  text-transform: uppercase;
  letter-spacing: 0.5px;
  margin-bottom: 0.25rem;
}

.info-section :deep(.tag-value) {
  font-size: 0.875rem;
  color: #212529;
  word-break: break-word;
}

.info-section :deep(.tag-item:last-child) {
  border-bottom: none;
  margin-bottom: 0;
  padding-bottom: 0;
}
</style>
