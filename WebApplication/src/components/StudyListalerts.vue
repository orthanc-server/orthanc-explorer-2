<script>
import { mapState, mapGetters } from "vuex";

export default {
  name: "StudyListAlerts",
  props: {
    isLoadingMostRecentStudies: { type: Boolean, default: false },
    isDisplayingMostRecentStudies: { type: Boolean, default: false },
    notShowingAllResults: { type: Boolean, default: false },
    showEmptyStudyListIfNoSearch: { type: Boolean, default: false },
    isStudyListEmpty: { type: Boolean, default: false },
    filterGenericTags: { type: Object, default: () => ({}) },
    filterStudyDate: { type: String, default: "" },
    filterPatientBirthDate: { type: String, default: "" },
    filterLabels: { type: Array, default: () => [] },
    activeModalityFilter: { type: [String, null], default: null },
    isCardView: { type: Boolean, default: false },
  },
  emits: [
    "clear-text-filter",
    "clear-birthdate-filter",
    "clear-labels-filter",
    "clear-all-filters",
    "clear-study-date-filter",
  ],
  data() {
    return {
      toasts: [],
      toastId: 0,
      lastToastTimes: {},
      // Флаг чтобы не показывать "нет результатов" сразу после смены фильтра
      justChangedFilter: false,
    };
  },
  computed: {
    ...mapState({
      isSearching: (state) => state.studies.isSearching,
    }),
    ...mapGetters(["studies/isFilterEmpty"]),

    studyDateDisplayText() {
      if (!this.filterStudyDate) return null;
      const today = new Date();
      today.setHours(0, 0, 0, 0);
      const fmt = (d) => {
        const y = d.getFullYear();
        const m = String(d.getMonth() + 1).padStart(2, "0");
        const day = String(d.getDate()).padStart(2, "0");
        return `${y}${m}${day}`;
      };
      if (this.filterStudyDate === fmt(today)) return this.$t("date_picker.today");
      const weekAgo = new Date(today); weekAgo.setDate(today.getDate() - 7);
      if (this.filterStudyDate === `${fmt(weekAgo)}-${fmt(today)}`) return this.$t("date_picker.last_week");
      const monthAgo = new Date(today); monthAgo.setMonth(today.getMonth() - 1);
      if (this.filterStudyDate === `${fmt(monthAgo)}-${fmt(today)}`) return this.$t("date_picker.last_month");
      const yearAgo = new Date(today); yearAgo.setFullYear(today.getFullYear() - 1);
      if (this.filterStudyDate === `${fmt(yearAgo)}-${fmt(today)}`) return this.$t("date_picker.last_12_months");
      if (this.filterStudyDate.includes("-")) {
        const [start, end] = this.filterStudyDate.split("-");
        return `${this.formatDicomDateShort(start)} - ${this.formatDicomDateShort(end)}`;
      }
      return this.formatDicomDateShort(this.filterStudyDate);
    },

    hasActiveTextFilters() {
      if (!this.filterGenericTags) return false;
      for (const [, value] of Object.entries(this.filterGenericTags)) {
        if (value && value.trim && value.trim() !== "") return true;
      }
      return false;
    },

    activeTextFiltersList() {
      const filters = [];
      const labelMap = {
        PatientName: this.$t("dicom_tags.PatientName"),
        PatientID: this.$t("dicom_tags.PatientID"),
        AccessionNumber: this.$t("dicom_tags.AccessionNumber"),
        StudyDescription: this.$t("dicom_tags.StudyDescription"),
        StudyID: this.$t("dicom_tags.StudyID"),
        StudyInstanceUID: "StudyUID",
        PatientSex: this.$t("dicom_tags.PatientSex"),
        OtherPatientIDs: this.$t("dicom_tags.OtherPatientIDs"),
        InstitutionName: this.$t("dicom_tags.InstitutionName"),
        ReferringPhysician: this.$t("dicom_tags.ReferringPhysicianName"),
        RequestingPhysician: this.$t("dicom_tags.RequestingPhysician"),
        ManufacturerModelName: this.$t("dicom_tags.ManufacturerModel"),
      };
      for (const [tag, value] of Object.entries(this.filterGenericTags)) {
        if (value && value.trim && value.trim() !== "") {
          filters.push({ tag, label: labelMap[tag] || tag, value });
        }
      }
      return filters;
    },

    hasActiveBirthDateFilter() {
      return !!this.filterPatientBirthDate && this.filterPatientBirthDate !== "";
    },

    formattedBirthDateFilter() {
      if (!this.filterPatientBirthDate) return "";
      if (this.filterPatientBirthDate.includes("-")) {
        const [start, end] = this.filterPatientBirthDate.split("-");
        return `${this.formatDicomDateShort(start)} - ${this.formatDicomDateShort(end)}`;
      }
      return this.formatDicomDateShort(this.filterPatientBirthDate);
    },

    hasAnyActiveFilters() {
      return (
        this.hasActiveTextFilters ||
        !!this.filterStudyDate ||
        this.hasActiveBirthDateFilter ||
        (this.filterLabels && this.filterLabels.length > 0) ||
        (!this.isCardView && this.activeModalityFilter)
      );
    },

    quickFilterAlertText() {
      if (this.filterStudyDate) return this.studyDateDisplayText;
      if (this.isDisplayingMostRecentStudies) return this.$t("displaying_most_recent_studies");
      return null;
    },

    showQuickFilterAlert() {
      return !this.isSearching && this.filterStudyDate;
    },

    // Проверяем, что список действительно пуст (не в процессе загрузки)
    shouldShowNoResults() {
      return (
        this.isStudyListEmpty &&
        !this.isSearching &&
        !this.isLoadingMostRecentStudies &&
        !this.justChangedFilter
      );
    },
  },
  watch: {
    // Следим за изменением фильтров даты
    filterStudyDate() {
      this.justChangedFilter = true;
      setTimeout(() => {
        this.justChangedFilter = false;
      }, 500);
    },
    // Следим за изменением текстовых фильтров
    filterGenericTags: {
      deep: true,
      handler() {
        this.justChangedFilter = true;
        setTimeout(() => {
          this.justChangedFilter = false;
        }, 500);
      },
    },
    notShowingAllResults(val, oldVal) {
      if (val && !oldVal) {
        this.showToastDebounced("not_showing_all_results", "danger");
      }
    },

    showEmptyStudyListIfNoSearch(val, oldVal) {
      if (val && !oldVal && this["studies/isFilterEmpty"] && !this.isSearching) {
        this.showToastDebounced("enter_search", "warning");
      }
    },

    shouldShowNoResults(val, oldVal) {
      if (val && !oldVal) {
        this.showToastDebounced("no_result_found", "warning");
      }
    },
  },
  methods: {
    countActiveFilters() {
      let count = 0;

      if (this.activeTextFiltersList) {
        count += this.activeTextFiltersList.length;
      }

      if (this.hasActiveBirthDateFilter) {
        count += 1;
      }

      if (this.filterLabels && this.filterLabels.length > 0) {
        count += 1;
      }

      return count;
    },
    showToastDebounced(key, type) {
      const now = Date.now();
      const lastTime = this.lastToastTimes[key];

      // Если тост с этим ключом уже был показан менее 5 сек назад — пропускаем
      if (lastTime && now - lastTime < 5000) {
        return;
      }

      // Сохраняем время и показываем тост
      this.lastToastTimes[key] = now;
      this.showToast(key, type);
    },
    formatDicomDateShort(dicomDate) {
      if (!dicomDate || dicomDate.length !== 8) return dicomDate;
      const year = dicomDate.substring(0, 4);
      const month = dicomDate.substring(4, 6);
      const day = dicomDate.substring(6, 8);
      return `${day}.${month}.${year}`;
    },
    showToast(messageKey, type) {
      const id = ++this.toastId;
      this.toasts.push({ id, message: this.$t(messageKey), type });
      setTimeout(() => {
        this.toasts = this.toasts.filter((t) => t.id !== id);
      }, 5000);
    },
    removeToast(id) {
      this.toasts = this.toasts.filter((t) => t.id !== id);
    },
  },
};
</script>

<template>
  <div class="alerts-container">
    <!-- Toast Notifications (только предупреждения) -->
    <TransitionGroup name="toast" tag="div" class="toast-container">
      <div v-for="toast in toasts" :key="toast.id" :class="`alert alert-${toast.type} toast-item`" role="alert">
        <i class="bi bi-exclamation-triangle-fill alert-icon"></i>
        {{ toast.message }}
        <!-- ИСПРАВЛЕНО: добавлен .stop и .prevent -->
        <i class="bi bi-x toast-close" @click.stop.prevent="removeToast(toast.id)"></i>
      </div>
    </TransitionGroup>

    <!-- Остальной шаблон без изменений -->
    <div v-if="hasAnyActiveFilters && !isSearching" class="active-filters-inline">
      <!-- Текстовые фильтры -->
      <span v-for="filter in activeTextFiltersList" :key="filter.tag" class="filter-tag"
        :title="filter.label + ': ' + filter.value" @click="$emit('clear-text-filter', filter.tag)">
        <i class="bi bi-search filter-tag-icon"></i>
        <span class="filter-tag-value">{{ filter.value }}</span>
        <i class="bi bi-x-circle filter-tag-close" :title="$t('clear_filter')"></i>
      </span>

      <!-- Фильтр по дате рождения -->
      <span v-if="hasActiveBirthDateFilter" class="filter-tag filter-tag-bd"
        :title="$t('dicom_tags.PatientBirthDate') + ': ' + formattedBirthDateFilter"
        @click="$emit('clear-birthdate-filter')">
        <i class="bi bi-calendar3 filter-tag-icon"></i>
        <span class="filter-tag-value">{{ formattedBirthDateFilter }}</span>
        <i class="bi bi-x-circle filter-tag-close" :title="$t('clear_filter')"></i>
      </span>
      <!-- Фильтр по дате исследования (quick filter) — теперь в едином стиле тега -->
      <span v-if="filterStudyDate" class="filter-tag filter-tag-studydate"
        :title="$t('study_date_filter') + ': ' + studyDateDisplayText" @click="$emit('clear-study-date-filter')">
        <i class="bi bi-calendar-range filter-tag-icon"></i>
        <span class="filter-tag-value">{{ studyDateDisplayText }}</span>
        <i class="bi bi-x-circle filter-tag-close" :title="$t('clear_filter')"></i>
      </span>
      <!-- Фильтр по меткам (labels) -->
      <span v-if="filterLabels && filterLabels.length > 0" class="filter-tag filter-tag-labels"
        :title="$t('labels.study_details_title') + ': ' + filterLabels.join(', ')"
        @click="$emit('clear-labels-filter')">
        <i class="bi bi-tags filter-tag-icon"></i>
        <span class="filter-tag-value">{{ filterLabels.join(', ') }}</span>
        <i class="bi bi-x-circle filter-tag-close" :title="$t('clear_filter')"></i>
      </span>

      <!-- Кнопка "Очистить все" -->
      <button v-if="countActiveFilters() > 2" class="btn btn-sm btn-clear-all" @click="$emit('clear-all-filters')"
        :title="$t('mobile_filters.clearAllF')">
        {{ $t('mobile_filters.clearAll') }}
      </button>
    </div>



    <!-- Информационные алерты -->
    <div v-if="!isSearching && isLoadingMostRecentStudies" class="alert alert-secondary study-list-alert" role="alert">
      <span class="spinner-border spinner-border-sm alert-icon" role="status" aria-hidden="true"></span>
      {{ $t("loading_most_recent_studies") }}
    </div>

    <div v-else-if="isSearching" class="alert alert-secondary study-list-alert" role="alert">
      <span class="spinner-border spinner-border-sm alert-icon" role="status" aria-hidden="true"></span>
      {{ $t("searching") }}
    </div>

    <!-- Quick Filter Alert -->
    <!-- <div v-if="showQuickFilterAlert" class="alert alert-secondary study-list-alert quick-filter-alert" role="alert">
      <i class="bi bi-calendar-range alert-icon"></i>
      <span class="quick-filter-text">{{ quickFilterAlertText }}</span>
      <i class="bi bi-x-circle quick-filter-close" @click.stop.prevent="$emit('clear-study-date-filter')"
        :title="$t('clear_filter')"></i>
    </div> -->
  </div>
</template>

<style scoped>
.toast-container {
  position: fixed;
  top: 20px;
  right: 20px;
  z-index: 1050;
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.toast-item {
  min-width: 300px;
  max-width: 400px;
  margin: 0;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 12px 16px;
}

.toast-close {
  margin-left: auto;
  cursor: pointer !important;
  opacity: 0.6;
  transition: opacity 0.2s;
  pointer-events: auto;
  /* Добавьте это */
  z-index: 1;
  /* И это */
}

.toast-close:hover {
  opacity: 1;
}

.toast-enter-active,
.toast-leave-active {
  transition: all 0.3s ease;
}

.toast-enter-from {
  opacity: 0;
  transform: translateX(100%);
}

.toast-leave-to {
  opacity: 0;
  transform: translateX(100%);
}

.study-list-alert {
  margin-top: var(--filter-margin);
  margin-bottom: var(--filter-margin);
  padding: 8px 12px;
  border-radius: 6px;
  font-size: 12px;
  font-weight: 500;
  display: flex;
  align-items: center;
  border: none;
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.1);
  justify-content: space-between;
}


.alert-secondary {
  background: linear-gradient(135deg, #64748b 0%, #475569 100%);
  color: white;
}

.alert-warning {
  background: linear-gradient(135deg, #f59e0b 0%, #d97706 100%);
  color: white;
}

.alert-danger {
  background: linear-gradient(135deg, #ef4444 0%, #dc2626 100%);
  color: white;
}

.alert-icon {
  margin-right: 8px;
  font-size: 14px;
}



.filter-tag {
  cursor: pointer;
  display: flex;
  background-color: rgb(156 163 169);
  margin: 5px;
  border-bottom: 2px solid rgba(255, 255, 255, 0.05);
  font-size: 10px;
  padding: 4px 6px !important;
  overflow: hidden;
  text-overflow: ellipsis;
  border-radius: 0 8px 8px 0;
  border-left: solid 2px rgba(255, 255, 255, 0.1);
  transition: all 0.2s ease;
  align-items: center;
  max-width: 120px;

}

span.filter-tag:hover {
  border-left: 3px solid #8b5cf6;
  transition: transform 0.2s;
  transform: translateX(2px)
}



.active-filters-inline {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0.5rem 1rem;

}

.filter-tag-bd {
  background-color: rgb(59 130 246);
  color: white;
}

.filter-tag-icon {
  margin-right: 6px;
  font-size: 12px;
}

.filter-tag-close {
  font-size: 12px;
  margin-left: 4px;
}

span.filter-tag-value {
  max-width: 70px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}




.btn-clear-all {
  background: none;
}

.btn-clear-all:hover {


  transition: transform 0.3s;
  font-weight: 600;
}

.btn-clear-all:active {


  border: none;
  transform: scale(0.95);
}

.filter-tag-studydate .filter-tag-value[data-v-07285275] {
  max-width: 100px;

}

.filter-tag.filter-tag-studydate {
  max-width: 150px !important;
}
</style>