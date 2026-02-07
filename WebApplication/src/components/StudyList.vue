<script>
/**
 * StudyList.vue (refactored)
 *
 * Оркестратор списка исследований.
 *
 * Обязанности:
 * - Хранение состояния фильтров (data)
 * - Бизнес-логика: загрузка/перезагрузка списка, управление URL, синхронизация с роутером
 * - Делегирование UI дочерним компонентам:
 *   • StudyListFilters  — thead (колонки + инпуты), quick-filters, sort, search button
 *   • StudyListAlerts   — алерты статуса, теги активных фильтров
 *
 * Взаимодействие с MobileFiltersModal:
 * - Принимает событие "apply-mobile-filters" и синхронизирует локальное состояние
 *   с тем, что применил мобильный фильтр.
 */
import StudyItem from "./StudyItem.vue";
import StudyCard from "./StudyCard.vue";
import ResourceButtonGroup from "./ResourceButtonGroup.vue";
import StudyListFilters from "./StudyListfilters.vue";
import StudyListAlerts from "./StudyListalerts.vue";
import Toasts from "./Toasts.vue";

import { mapState, mapGetters } from "vuex";
import { translateDicomTag } from "../locales/i18n";
import dateHelpers from "../helpers/date-helpers";
import $ from "jquery";
import api from "../orthancApi";
import SourceType from "../helpers/source-type";
import { ObserveVisibility as vObserveVisibility } from "vue3-observe-visibility";
import { nextTick } from "vue";

// ─── Constants ────────────────────────────────────────────────────────
const Status = Object.freeze({
  UNDEFINED: 0,
  LOADING_MOST_RECENT_STUDIES: 1,
  DISPLAYING_MOST_RECENT_STUDIES: 2,
  DISPLAYING_FILTERED_STUDIES: 3,
});

document._allowedFilters = [
  "StudyDate", "StudyTime", "AccessionNumber", "PatientID", "PatientName",
  "PatientBirthDate", "StudyInstanceUID", "StudyID", "StudyDescription",
  "ModalitiesInStudy", "labels",
];

document._studyColumns = {
  StudyDate: { width: "7%", isOrderable: true },
  AccessionNumber: { width: "10%", placeholder: "AE1234", isOrderable: true },
  PatientID: { width: "11%", placeholder: "ID1234", isOrderable: true },
  PatientName: { width: "20%", placeholder: "Dementev^Ivan", isOrderable: true },
  PatientBirthDate: { width: "7%", isOrderable: true },
  StudyDescription: { width: "15%", placeholder: "CHEST", isOrderable: true },
  modalities: { width: "6%", isOrderable: false },
  seriesCount: { width: "4%", isOrderable: false },
  instancesCount: { width: "4%", isOrderable: false },
  seriesAndInstancesCount: { width: "7%", isOrderable: false },
  undefined: { width: "10%", isOrderable: false },
  PatientSex: { width: "7%", placeholder: "", isOrderable: true },
  OtherPatientIDs: { width: "10%", placeholder: "", isOrderable: true },
  InstitutionName: { width: "12%", placeholder: "", isOrderable: true },
  ReferringPhysician: { width: "10%", placeholder: "", isOrderable: true },
  RequestingPhysician: { width: "10%", placeholder: "", isOrderable: true },
  ManufacturerModelName: { width: "10%", placeholder: "", isOrderable: true },
};

// ─── Component ────────────────────────────────────────────────────────
export default {
  props: [],
  emits: ["deletedStudy"],
  data() {
    return {
      // View
      viewMode: localStorage.getItem("studyViewMode") || "table",
      isMobilePortrait: false,
      cardViewSort: "date-desc",

      // Filters
      filterStudyDate: "",
      filterStudyDateForDatePicker: "",
      filterPatientBirthDate: "",
      filterPatientBirthDateForDatePicker: "",
      filterModalities: {},
      filterGenericTags: {},
      oldFilterGenericTags: {},
      filterLabels: [],
      activeModalityFilter: null,

      // Ordering
      currentOrderByTag: null,
      currentOrderDirection: "ASC",
      filterOrderBy: [
        { Type: "Metadata", Key: "LastUpdate", Direction: "DESC" },
      ],

      // Modalities UI
      allModalities: true,
      noneModalities: false,

      // Internal flags
      updatingFilterUi: false,
      updatingRouteWithoutReload: false,
      initializingModalityFilter: false,
      searchTimerHandler: {},

      // ЗАЩИТА ОТ ЗАЦИКЛИВАНИЯ
      isSyncingFromStore: false,
      lastFilterUpdateTime: 0,

      // Config refs
      columns: document._studyColumns,
      datePickerPresetRanges: document._datePickerPresetRanges,

      // Selection
      allSelected: false,
      isPartialSelected: false,

      // Loading state
      mostRecentStudiesIds: [],
      shouldStopLoadingMostRecentStudies: false,
      status: Status.UNDEFINED,

      // Source
      sourceType: SourceType.LOCAL_ORTHANC,
      remoteSource: null,

      // Labels filter
      showMultiLabelsFilter: false,
      multiLabelsFilterLabelsConstraint: "All",
      showStudiesWithoutLabels: false,
      multiLabelsComponentKey: 0,
    };
  },

  // ─── watch ────────────────────────────────────────────────────────────
  watch: {
    $route() {
      if (!this.updatingRouteWithoutReload) this.updateFilterFromRoute(this.$route.query);
    },

    isConfigurationLoaded() {
      this.initModalityFilter();
      for (const tag of this.uiOptions.StudyListColumns) {
        if (!["StudyDate", "PatientBirthDate", "modalities", "seriesCount", "instancesCount", "seriesAndInstancesCount"].includes(tag)) {
          this.filterGenericTags[tag] = "";
        }
      }
      this.updateFilterFromRoute(this.$route.query);
      setTimeout(() => { this.showMultiLabelsFilter = true; }, 300);
    },

    filterModalities: {
      handler() {
        if (!this.updatingFilterUi && !this.initializingModalityFilter) {
          if (this.isSearchAsYouTypeEnabled) {
            this.updateFilter("ModalitiesInStudy", this.getModalityFilter(), null);
          } else {
            this.getModalityFilter();
          }
        }
      },
      deep: true,
    },

    filterGenericTags: {
      deep: true,
      handler() {
        // Пропускаем если синхронизируем из store (чтобы не уйти в рекурсию)
        if (this.isSyncingFromStore) return;

        for (const [k, v] of Object.entries(this.filterGenericTags)) {
          const old = this.oldFilterGenericTags[k] ?? null;

          // Обновляем только если значение реально изменилось
          if (v !== old) {
            this.updateFilter(k, v, old);
            this.oldFilterGenericTags[k] = v;
          }
        }
      },
    },

    filterStudyDate(newValue, oldValue) {
      // Пропускаем если синхронизируем из store
      if (this.isSyncingFromStore) return;
      if (!this.updatingFilterUi) this.updateFilter("StudyDate", newValue, oldValue);
    },

    filterStudyDateForDatePicker(newValue) {
      let d = dateHelpers.dicomDateFromDatePicker(newValue);
      this.filterStudyDate = d ?? "";
    },

    filterPatientBirthDate(newValue, oldValue) {
      // Пропускаем если синхронизируем из store
      if (this.isSyncingFromStore) return;
      this.updateFilter("PatientBirthDate", newValue, oldValue);
    },

    filterPatientBirthDateForDatePicker(newValue) {
      let d = dateHelpers.dicomDateFromDatePicker(newValue);
      this.filterPatientBirthDate = d ?? "";
    },

    async multiLabelsFilterLabelsConstraint(newValue) {
      if (!this.updatingFilterUi && this.isSearchAsYouTypeEnabled && !this.isSyncingFromStore) {
        const labels = newValue === "None" ? [] : this.filterLabels;
        await this.$store.dispatch("studies/updateLabelFilterNoReload", {
          labels,
          constraint: this.multiLabelsFilterLabelsConstraint,
        });
        this.updateUrlNoReload();
        this.reloadStudyList();
      }
    },

    selectedStudiesIds: {
      handler() { this.updateSelectAll(); },
      deep: true,
    },

    allLabels() { this.multiLabelsComponentKey++; },

    '$store.state.studies.dicomTagsFilters': {
      handler(newFilters) {
        if (this.updatingFilterUi) return;

        if (newFilters) {
          // Устанавливаем флаг чтобы watchers не среагировали на изменения
          this.isSyncingFromStore = true;

          // StudyDate
          if (newFilters.StudyDate !== this.filterStudyDate) {
            this.filterStudyDate = newFilters.StudyDate || "";
            this.filterStudyDateForDatePicker = newFilters.StudyDate
              ? dateHelpers.parseDateForDatePicker(newFilters.StudyDate)
              : null;
          }

          // PatientBirthDate
          if (newFilters.PatientBirthDate !== this.filterPatientBirthDate) {
            this.filterPatientBirthDate = newFilters.PatientBirthDate || "";
            this.filterPatientBirthDateForDatePicker = newFilters.PatientBirthDate
              ? dateHelpers.parseDateForDatePicker(newFilters.PatientBirthDate)
              : null;
          }

          // Текстовые поля - обновляем только изменившиеся
          for (const tag of ["PatientName", "PatientID", "AccessionNumber", "StudyDescription"]) {
            if (newFilters[tag] !== this.filterGenericTags[tag]) {
              this.filterGenericTags[tag] = newFilters[tag] || "";
              this.oldFilterGenericTags[tag] = newFilters[tag] || "";
            }
          }

          // ModalitiesInStudy
          if (newFilters.ModalitiesInStudy !== undefined) {
            const currentModalityFilter = this.getModalityFilter();
            if (newFilters.ModalitiesInStudy !== currentModalityFilter) {
              if (newFilters.ModalitiesInStudy === "NONE") {
                for (const mod of this.uiOptions.ModalitiesFilter) {
                  this.filterModalities[mod] = false;
                }
                this.allModalities = false;
                this.noneModalities = true;
                this.activeModalityFilter = null;
              } else if (newFilters.ModalitiesInStudy) {
                const selected = newFilters.ModalitiesInStudy.split("\\");
                this.activeModalityFilter = selected.join(", ");
                for (const mod of this.uiOptions.ModalitiesFilter) {
                  this.filterModalities[mod] = selected.includes(mod);
                }
                this.allModalities = false;
                this.noneModalities = false;
              } else {
                for (const mod of this.uiOptions.ModalitiesFilter) {
                  this.filterModalities[mod] = true;
                }
                this.allModalities = true;
                this.noneModalities = false;
                this.activeModalityFilter = null;
              }
            }
          }

          // Сбрасываем флаг после обработки всех изменений
          nextTick(() => {
            this.isSyncingFromStore = false;
          });
        }
      },
      deep: true,
      immediate: true
    }
  },


  // ─── Computed ───────────────────────────────────────────────────────
  computed: {
    ...mapState({
      uiOptions: (state) => state.configuration.uiOptions,
      allLabels: (state) => state.labels.allLabels,
      isConfigurationLoaded: (state) => state.configuration.loaded,
      studiesIds: (state) => state.studies.studiesIds,
      selectedStudiesIds: (state) => state.studies.selectedStudiesIds,
      isSearching: (state) => state.studies.isSearching,
      statistics: (state) => state.studies.statistics,
      hasExtendedFind: (state) => state.configuration.hasExtendedFind,
      hasExtendedChanges: (state) => state.configuration.hasExtendedChanges,
      canShowStudiesWithoutLabels: (state) => state.labels.canShowStudiesWithoutLabels,
    }),
    ...mapGetters(["studies/isFilterEmpty", "studies/isMostRecentOrdering"]),

    // --- View mode ---
    isCardView() {
      return this.isMobilePortrait || this.viewMode === "cards";
    },
    isTableView() {
      return !this.isMobilePortrait && this.viewMode === "table";
    },

    // --- Card sort options ---
    cardViewSortOptions() {
      return [
        { value: "date-desc", label: "↓📅", title: this.$t("sort_date_newest") },
        { value: "date-asc", label: "↑📅", title: this.$t("sort_date_oldest") },
        { value: "patient-asc", label: "A→Z 👤", title: this.$t("sort_patient_az") },
        { value: "patient-desc", label: "Z→A 👤", title: this.$t("sort_patient_za") },
        { value: "modality", label: "🩻", title: this.$t("sort_modality") },
        { value: "images-desc", label: "🖼️↓", title: this.$t("sort_images_most") },
      ];
    },

    // --- Sorted studies for card view ---
    sortedStudiesForCards() {
      if (!this.isCardView) return this.studiesIds;
      const studies = this.$store.state.studies.studies;
      const studyMap = new Map();
      for (const study of studies) studyMap.set(study.ID, study);
      const sorted = [...this.studiesIds];
      const get = (id) => studyMap.get(id);
      switch (this.cardViewSort) {
        case "date-desc":
          sorted.sort((a, b) => (get(b)?.MainDicomTags?.StudyDate || "").localeCompare(get(a)?.MainDicomTags?.StudyDate || ""));
          break;
        case "date-asc":
          sorted.sort((a, b) => (get(a)?.MainDicomTags?.StudyDate || "").localeCompare(get(b)?.MainDicomTags?.StudyDate || ""));
          break;
        case "patient-asc":
          sorted.sort((a, b) => (get(a)?.PatientMainDicomTags?.PatientName || "").localeCompare(get(b)?.PatientMainDicomTags?.PatientName || ""));
          break;
        case "patient-desc":
          sorted.sort((a, b) => (get(b)?.PatientMainDicomTags?.PatientName || "").localeCompare(get(a)?.PatientMainDicomTags?.PatientName || ""));
          break;
        case "modality":
          sorted.sort((a, b) => (get(a)?.RequestedTags?.ModalitiesInStudy || "").localeCompare(get(b)?.RequestedTags?.ModalitiesInStudy || ""));
          break;
        case "images-desc":
          sorted.sort((a, b) => parseInt(get(b)?.RequestedTags?.NumberOfStudyRelatedInstances || "0") - parseInt(get(a)?.RequestedTags?.NumberOfStudyRelatedInstances || "0"));
          break;
      }
      return sorted;
    },

    // --- Table column helpers ---
    hasQuickActionsColumn() {
      return this.sourceType === SourceType.LOCAL_ORTHANC && (this.uiOptions.EnableDownloadQuickButton || this.uiOptions.EnableSendQuickButton);
    },
    hasPrimaryViewerIcon() {
      return this.sourceType === SourceType.LOCAL_ORTHANC && this.uiOptions.EnableViewerQuickButton;
    },
    hasPdfReportIcon() {
      return this.sourceType === SourceType.LOCAL_ORTHANC && this.uiOptions.EnableReportQuickButton;
    },

    // --- Status ---
    notShowingAllResults() {
      if (this.sourceType === SourceType.LOCAL_ORTHANC && !this.hasExtendedFind) {
        if (this.studiesIds.length >= this.statistics.CountStudies) return false;
        return this.studiesIds.length === this.uiOptions.MaxStudiesDisplayed;
      }
      return false;
    },
    isDisplayingMostRecentStudies() {
      return this.status === Status.DISPLAYING_MOST_RECENT_STUDIES;
    },
    isLoadingMostRecentStudies() {
      return this.status === Status.LOADING_MOST_RECENT_STUDIES;
    },
    isDarkMode() {
      let bsTheme = getComputedStyle(document.documentElement).getPropertyValue("--bootstrap-theme");
      return bsTheme === "dark";
    },

    // --- Remote ---
    isRemoteDicom() { return this.sourceType === SourceType.REMOTE_DICOM; },
    isRemoteDicomWeb() { return this.sourceType === SourceType.REMOTE_DICOM_WEB; },

    // --- Search/Filter mode ---
    isMultiLabelsFilterVisible() {
      return this.sourceType === SourceType.LOCAL_ORTHANC && this.showMultiLabelsFilter && this.uiOptions.EnableMultiLabelsSearch;
    },
    isSearchAsYouTypeEnabled() {
      return this.sourceType === SourceType.LOCAL_ORTHANC && this.uiOptions.StudyListSearchMode === "search-as-you-type";
    },
    isSearchButtonEnabled() {
      return this.sourceType === SourceType.LOCAL_ORTHANC
        ? this.uiOptions.StudyListSearchMode === "search-button"
        : true;
    },
    showEmptyStudyListIfNoSearch() {
      return this.sourceType === SourceType.LOCAL_ORTHANC
        ? this.uiOptions.StudyListContentIfNoSearch === "empty"
        : true;
    },
    isStudyListEmpty() { return this.studiesIds.length === 0; },
    datePickerFormat() { return this.uiOptions.DateFormat; },
    selectedStudiesCount() { return this.selectedStudiesIds.length > 0 ? this.selectedStudiesIds.length : ""; },

    // --- ColSpans for labels filter row ---
    colSpanBeforeMultiLabelsFilter() {
      let s = 1;
      if (this.hasPrimaryViewerIcon) s++;
      if (this.hasPdfReportIcon) s++;
      return s;
    },
    colSpanMultiLabelsFilter() {
      if (this.uiOptions?.StudyListColumns) {
        let total = this.uiOptions.StudyListColumns.length + 1;
        if (this.hasPrimaryViewerIcon) total++;
        if (this.hasPdfReportIcon) total++;
        return total - this.colSpanBeforeMultiLabelsFilter - this.colSpanAfterMultiLabelsFilter;
      }
      return 4;
    },
    colSpanAfterMultiLabelsFilter() { return 3; },
    widthColum1() { return this.colSpanClearFilter === 1 ? "4%" : "3%"; },
    colSpanClearFilter() {
      if (this.sourceType !== SourceType.LOCAL_ORTHANC) return 1;
      let s = 1;
      if (this.hasPrimaryViewerIcon) s++;
      if (this.hasPdfReportIcon) s++;
      return s;
    },
  },

  // ─── Watchers ───────────────────────────────────────────────────────
  watch: {
    $route() {
      if (!this.updatingRouteWithoutReload) this.updateFilterFromRoute(this.$route.query);
    },
    isConfigurationLoaded() {
      this.initModalityFilter();
      for (const tag of this.uiOptions.StudyListColumns) {
        if (!["StudyDate", "PatientBirthDate", "modalities", "seriesCount", "instancesCount", "seriesAndInstancesCount"].includes(tag)) {
          this.filterGenericTags[tag] = "";
        }
      }
      this.updateFilterFromRoute(this.$route.query);
      setTimeout(() => { this.showMultiLabelsFilter = true; }, 300);
    },
    filterModalities: {
      handler() {
        if (!this.updatingFilterUi && !this.initializingModalityFilter) {
          if (this.isSearchAsYouTypeEnabled) {
            this.updateFilter("ModalitiesInStudy", this.getModalityFilter(), null);
          } else {
            this.getModalityFilter();
          }
        }
      },
      deep: true,
    },
    filterGenericTags: {
      handler() {
        for (const [k, v] of Object.entries(this.filterGenericTags)) {
          const old = this.oldFilterGenericTags[k] ?? null;
          this.updateFilter(k, v, old);
          this.oldFilterGenericTags[k] = v;
        }
      },
      deep: true,
    },
    filterStudyDate(newValue, oldValue) {
      if (!this.updatingFilterUi) this.updateFilter("StudyDate", newValue, oldValue);
    },
    filterStudyDateForDatePicker(newValue) {
      let d = dateHelpers.dicomDateFromDatePicker(newValue);
      this.filterStudyDate = d ?? "";
    },
    filterPatientBirthDate(newValue, oldValue) {
      this.updateFilter("PatientBirthDate", newValue, oldValue);
    },
    filterPatientBirthDateForDatePicker(newValue) {
      let d = dateHelpers.dicomDateFromDatePicker(newValue);
      this.filterPatientBirthDate = d ?? "";
    },
    async multiLabelsFilterLabelsConstraint(newValue) {
      if (!this.updatingFilterUi && this.isSearchAsYouTypeEnabled) {
        const labels = newValue === "None" ? [] : this.filterLabels;
        await this.$store.dispatch("studies/updateLabelFilterNoReload", {
          labels,
          constraint: this.multiLabelsFilterLabelsConstraint,
        });
        this.updateUrlNoReload();
        this.reloadStudyList();
      }
    },
    selectedStudiesIds: {
      handler() { this.updateSelectAll(); },
      deep: true,
    },
    allLabels() { this.multiLabelsComponentKey++; },
    '$store.state.studies.dicomTagsFilters': {
      handler(newFilters) {
        if (this.updatingFilterUi) return;

        // Синхронизируем ВСЕ фильтры из store
        if (newFilters) {
          // StudyDate
          if (newFilters.StudyDate !== this.filterStudyDate) {
            this.filterStudyDate = newFilters.StudyDate || "";
            this.filterStudyDateForDatePicker = newFilters.StudyDate
              ? dateHelpers.parseDateForDatePicker(newFilters.StudyDate)
              : null;
          }

          // PatientBirthDate
          if (newFilters.PatientBirthDate !== this.filterPatientBirthDate) {
            this.filterPatientBirthDate = newFilters.PatientBirthDate || "";
            this.filterPatientBirthDateForDatePicker = newFilters.PatientBirthDate
              ? dateHelpers.parseDateForDatePicker(newFilters.PatientBirthDate)
              : null;
          }

          // Текстовые поля
          for (const tag of ["PatientName", "PatientID", "AccessionNumber", "StudyDescription"]) {
            if (newFilters[tag] !== this.filterGenericTags[tag]) {
              this.filterGenericTags[tag] = newFilters[tag] || "";
            }
          }

          // ModalitiesInStudy
          if (newFilters.ModalitiesInStudy !== undefined) {
            const currentModalityFilter = this.getModalityFilter();
            if (newFilters.ModalitiesInStudy !== currentModalityFilter) {
              if (newFilters.ModalitiesInStudy === "NONE") {
                for (const mod of this.uiOptions.ModalitiesFilter) {
                  this.filterModalities[mod] = false;
                }
                this.allModalities = false;
                this.noneModalities = true;
                this.activeModalityFilter = null;
              } else if (newFilters.ModalitiesInStudy) {
                const selected = newFilters.ModalitiesInStudy.split("\\");
                this.activeModalityFilter = selected.join(", ");
                for (const mod of this.uiOptions.ModalitiesFilter) {
                  this.filterModalities[mod] = selected.includes(mod);
                }
                this.allModalities = false;
                this.noneModalities = false;
              } else {
                // Все выбраны
                for (const mod of this.uiOptions.ModalitiesFilter) {
                  this.filterModalities[mod] = true;
                }
                this.allModalities = true;
                this.noneModalities = false;
                this.activeModalityFilter = null;
              }
            }
          }
        }
      },
      deep: true,
      immediate: true // Важно: запустить сразу при создании
    }
  },

  // ─── Lifecycle ──────────────────────────────────────────────────────
  async created() {
    this.messageBus.on("language-changed", this.translateDatePicker);
    this.messageBus.on('filters-applied-from-mobile', this.syncFromMobileFilters);
    window.addEventListener("mobile-portrait-mode", this.handleMobilePortraitMode);
    this.checkMobilePortrait();
    if (this.isConfigurationLoaded) {
      setTimeout(() => { this.showMultiLabelsFilter = true; }, 300);
    }
  },
  async mounted() {
    window.addEventListener("resize", this.checkMobilePortrait);
  },
  beforeUnmount() {
    this.messageBus.off('filters-applied-from-mobile', this.syncFromMobileFilters); // ДОБАВИТЬ

    window.removeEventListener("mobile-portrait-mode", this.handleMobilePortraitMode);
    window.removeEventListener("resize", this.checkMobilePortrait);
  },

  // ─── Methods ────────────────────────────────────────────────────────
  methods: {
    clearStudyDateFilter() {
      this.updatingFilterUi = true;
      this.filterStudyDate = "";
      this.filterStudyDateForDatePicker = null;
      this.$store.dispatch("studies/updateFilterNoReload", { dicomTagName: "StudyDate", value: "" });
      this.updateUrlNoReload();
      this.reloadStudyList();
      nextTick(() => { this.updatingFilterUi = false; });
    },

    checkMobilePortrait() {
      const isMobile = window.matchMedia("(max-width: 768px)").matches;
      const isPortrait = window.matchMedia("(orientation: portrait)").matches;
      this.isMobilePortrait = isMobile && isPortrait;
      if (this.isMobilePortrait) this.viewMode = "cards";
    },

    handleMobilePortraitMode(e) {
      this.isMobilePortrait = e.detail.isMobilePortrait;
      if (this.isMobilePortrait) this.viewMode = "cards";
    },

    setViewMode(mode) {
      this.viewMode = mode;
      localStorage.setItem("studyViewMode", mode);
    },

    updateSelectAll() {
      if (this.selectedStudiesIds.length === 0) {
        this.allSelected = false;
        this.isPartialSelected = false;
      } else if (this.selectedStudiesIds.length === this.studiesIds.length) {
        this.allSelected = true;
        this.isPartialSelected = false;
      } else {
        this.allSelected = "";
        this.isPartialSelected = true;
      }
    },

    clickSelectAll() {
      if (this.allSelected === "" || !this.allSelected) {
        this.$store.dispatch("studies/selectAllStudies", { isSelected: true });
        this.messageBus.emit("selected-all");
      } else {
        this.$store.dispatch("studies/selectAllStudies", { isSelected: false });
        this.messageBus.emit("unselected-all");
      }
    },

    translateDatePicker() {
      for (let i in document._datePickerPresetRanges) {
        document._datePickerPresetRanges[i].label = this.$t(document._datePickerPresetRanges[i].tLabel);
      }
    },

    initModalityFilter() {
      this.initializingModalityFilter = true;
      this.filterModalities = {};
      for (const mod of this.uiOptions.ModalitiesFilter) {
        this.filterModalities[mod] = true;
      }
      this.initializingModalityFilter = false;
    },

    getModalityFilter() {
      if (!this.filterModalities) return "";
      let allSel = true;
      const selected = [];
      for (const [key, value] of Object.entries(this.filterModalities)) {
        allSel &= value;
        if (value) selected.push(key);
      }
      if (allSel) { this.allModalities = true; this.noneModalities = false; return ""; }
      if (selected.length === 0) { this.allModalities = false; this.noneModalities = true; return "NONE"; }
      this.allModalities = false; this.noneModalities = false;
      return selected.join("\\");
    },

    clearModalityFilter() {
      if (this.uiOptions.StudyListColumns) {
        for (const mod of this.uiOptions.ModalitiesFilter) {
          this.filterModalities[mod] = true;
        }
      }
    },

    clearActiveModalityFilter() {
      this.activeModalityFilter = null;
      for (const mod of this.uiOptions.ModalitiesFilter) this.filterModalities[mod] = true;
      this.allModalities = true;
      this.noneModalities = false;
      this.$store.dispatch("studies/updateFilterNoReload", { dicomTagName: "ModalitiesInStudy", value: "" });
      this.updateUrlNoReload();
      this.reloadStudyList();
    },

    onFilterModality(modality) {
      const mods = modality.split(/[\\,]+/).map(m => m.trim()).filter(m => m);
      if (!mods.length) return;
      this.activeModalityFilter = mods.join(", ");
      for (const key of Object.keys(this.filterModalities)) this.filterModalities[key] = false;
      for (const mod of mods) { if (mod in this.filterModalities) this.filterModalities[mod] = true; }
      this.allModalities = false;
      this.noneModalities = false;
      this.$store.dispatch("studies/updateFilterNoReload", { dicomTagName: "ModalitiesInStudy", value: mods.join("\\") });
      this.updateUrlNoReload();
      this.reloadStudyList();
    },

    handleToggleModalityFilter(ev) {
      const modality = ev.srcElement.getAttribute("data-value");
      const newValue = modality === "all";
      for (const key of Object.keys(this.filterModalities)) this.filterModalities[key] = newValue;
      this.getModalityFilter();
    },

    handleToggleOrder(tagName) {
      if (this.currentOrderByTag === tagName) {
        this.currentOrderDirection = this.currentOrderDirection === "ASC" ? "DESC" : "ASC";
      } else {
        this.currentOrderByTag = tagName;
        this.currentOrderDirection = "ASC";
      }
      const o = { Type: "DicomTag", Key: this.currentOrderByTag, Direction: this.currentOrderDirection };
      this.filterOrderBy = [o].concat(this.filterOrderBy.filter(i => i.Type !== "DicomTag"));
      this._updateOrderBy(true);
    },

    _updateOrderBy(reloadNow) {
      this.$store.dispatch("studies/updateOrderByNoReload", { orderBy: this.filterOrderBy });
      if (reloadNow) { this.updateUrlNoReload(); this.reloadStudyList(); }
    },

    updateOrderBy(orderString, reloadNow) {
      this.filterOrderBy = [];
      this.currentOrderByTag = null;
      this.currentOrderDirection = "ASC";
      for (const order of orderString.split(";")) {
        const o = order.split(",");
        this.filterOrderBy.push({ Type: o[0], Key: o[1], Direction: o[2] });
        if (o[0] === "DicomTag" && this.currentOrderByTag == null) {
          this.currentOrderByTag = o[1];
          this.currentOrderDirection = o[2];
        }
      }
      this._updateOrderBy(reloadNow);
    },

    updateFilter(dicomTagName, newValue, oldValue) {
      if (this.updatingFilterUi || !this.isSearchAsYouTypeEnabled) return;

      // Защита от слишком частых вызовов
      const now = Date.now();
      const timeSinceLastUpdate = now - this.lastFilterUpdateTime;

      // Если вызов слишком частый (менее 50мс), откладываем
      if (timeSinceLastUpdate < 50) {
        if (this.searchTimerHandler[dicomTagName]) {
          clearTimeout(this.searchTimerHandler[dicomTagName]);
        }
      }
      this.lastFilterUpdateTime = now;

      if (dicomTagName === "ModalitiesInStudy" && oldValue == null) {
        this._updateFilter(dicomTagName, newValue);
        return;
      }

      if (newValue.length >= this.uiOptions.StudyListSearchAsYouTypeMinChars) {
        if (this.searchTimerHandler[dicomTagName]) clearTimeout(this.searchTimerHandler[dicomTagName]);
        this.searchTimerHandler[dicomTagName] = setTimeout(() => {
          this._updateFilter(dicomTagName, newValue);
        }, this.uiOptions.StudyListSearchAsYouTypeDelay);
      } else if (oldValue && newValue.length < oldValue.length && oldValue.length >= this.uiOptions.StudyListSearchAsYouTypeMinChars) {
        if (this.searchTimerHandler[dicomTagName]) clearTimeout(this.searchTimerHandler[dicomTagName]);
        this.searchTimerHandler[dicomTagName] = setTimeout(() => {
          this._updateFilter(dicomTagName, "");
        }, this.uiOptions.StudyListSearchAsYouTypeDelay);
      }
    },

    _updateFilter(dicomTagName, value) {
      this.searchTimerHandler[dicomTagName] = null;
      this.$store.dispatch("studies/updateFilterNoReload", { dicomTagName, value });
      this.updateUrlNoReload();
      this.reloadStudyList();
    },

    clipFilter(dicomTagName, value) {
      return this.isFilterLongEnough(dicomTagName, value) ? value : "";
    },

    getMinimalFilterLength(dicomTagName) {
      if (["AccessionNumber", "PatientName", "PatientID", "StudyDescription"].includes(dicomTagName)) {
        return this.isSearchAsYouTypeEnabled ? this.uiOptions.StudyListSearchAsYouTypeMinChars : 0;
      }
      if (["PatientBirthDate", "StudyDate"].includes(dicomTagName)) return 8;
      return 0;
    },

    isFilterLongEnough(dicomTagName, value) {
      return value.length >= this.getMinimalFilterLength(dicomTagName);
    },

    getFilterValue(dicomTagName) {
      if (!this.isConfigurationLoaded) return null;
      if (dicomTagName === "StudyDate") return this.filterStudyDate;
      if (dicomTagName === "PatientBirthDate") return this.filterPatientBirthDate;
      return this.filterGenericTags[dicomTagName];
    },

    clearTextFilter(tagName) {
      // Очищаем pending таймаут для этого тега
      if (this.searchTimerHandler[tagName]) {
        clearTimeout(this.searchTimerHandler[tagName]);
        this.searchTimerHandler[tagName] = null;
      }

      this.filterGenericTags[tagName] = "";
      this.oldFilterGenericTags[tagName] = "";
      this._updateFilter(tagName, "");
    },

    clearBirthDateFilter() {
      // Очищаем pending таймаут
      if (this.searchTimerHandler["PatientBirthDate"]) {
        clearTimeout(this.searchTimerHandler["PatientBirthDate"]);
        this.searchTimerHandler["PatientBirthDate"] = null;
      }

      this.filterPatientBirthDate = "";
      this.filterPatientBirthDateForDatePicker = null;
      this._updateFilter("PatientBirthDate", "");
    },

    clearLabelsFilter() {
      this.filterLabels = [];
      this.$store.dispatch("studies/updateLabelFilterNoReload", { labels: [], constraint: "All" });
      this.updateUrlNoReload();
      this.reloadStudyList();
    },

    async clearFilters() {
      await this.clearFiltersUi();
      await this.$store.dispatch("studies/clearFilterNoReload");
      this.activeModalityFilter = null;
      if (this.sourceType === SourceType.LOCAL_ORTHANC) this.reloadStudyList();
    },

    async clearFiltersUi() {
      this.updatingFilterUi = true;
      this.emptyFilterForm();
      this.activeModalityFilter = null;
      this.filterStudyDateForDatePicker = null;
      this.updateUrl();
      await nextTick();
      this.updatingFilterUi = false;
    },

    emptyFilterForm() {
      this.filterStudyDate = "";
      this.filterStudyDateForDatePicker = null;
      this.filterPatientBirthDate = "";
      this.multiLabelsFilterLabelsConstraint = "All";
      this.showStudiesWithoutLabels = false;
      this.filterPatientBirthDateForDatePicker = null;
      this.filterGenericTags = {};
      if (this.uiOptions.StudyListColumns) {
        for (const tag of this.uiOptions.StudyListColumns) {
          if (!["StudyDate", "PatientBirthDate", "modalities", "seriesCount", "instancesCount", "seriesAndInstancesCount"].includes(tag)) {
            this.filterGenericTags[tag] = "";
          }
        }
      }
      this.filterLabels = [];
      this.clearModalityFilter();
    },

    async search() {
      if (this.isSearching) {
        await this.$store.dispatch("studies/cancelSearch");
      } else {
        await this.$store.dispatch("studies/clearFilterNoReload");
        for (const tag of this.uiOptions.StudyListColumns) {
          if (!["modalities", "seriesCount", "instancesCount", "seriesAndInstancesCount"].includes(tag)) {
            await this.$store.dispatch("studies/updateFilterNoReload", { dicomTagName: tag, value: this.getFilterValue(tag) });
          }
        }
        await this.$store.dispatch("studies/updateFilterNoReload", { dicomTagName: "ModalitiesInStudy", value: this.getModalityFilter() });
        if (this.showStudiesWithoutLabels) {
          await this.$store.dispatch("studies/updateLabelFilterNoReload", { labels: [], constraint: "None" });
        } else {
          await this.$store.dispatch("studies/updateLabelFilterNoReload", { labels: this.filterLabels, constraint: this.multiLabelsFilterLabelsConstraint });
        }
        await this.updateUrlNoReload();
        await this.reloadStudyList();
      }
    },

    async setCardQuickFilter(filterType) {
      const today = new Date();
      today.setHours(0, 0, 0, 0);
      const fmt = (d) => {
        const y = d.getFullYear();
        const m = String(d.getMonth() + 1).padStart(2, "0");
        const day = String(d.getDate()).padStart(2, "0");
        return `${y}${m}${day}`;
      };

      let dateValue = "";
      switch (filterType) {
        case "today": dateValue = fmt(today); break;
        case "week": { const d = new Date(today); d.setDate(today.getDate() - 7); dateValue = `${fmt(d)}-${fmt(today)}`; break; }
        case "month": { const d = new Date(today); d.setMonth(today.getMonth() - 1); dateValue = `${fmt(d)}-${fmt(today)}`; break; }
        case "year": { const d = new Date(today); d.setFullYear(today.getFullYear() - 1); dateValue = `${fmt(d)}-${fmt(today)}`; break; }
        default: dateValue = ""; break;
      }

      this.updatingFilterUi = true;
      this.filterStudyDate = dateValue;
      this.filterStudyDateForDatePicker = dateValue ? dateHelpers.parseDateForDatePicker(dateValue) : null;
      await nextTick();
      this.updatingFilterUi = false;

      this.$store.dispatch("studies/updateFilterNoReload", { dicomTagName: "StudyDate", value: dateValue });
      this.updateUrlNoReload();
      this.reloadStudyList();
    },

    onMultiLabelsFilterChanged(newValues) {
      if (!this.updatingFilterUi) {
        this.filterLabels = newValues;
        if (this.isSearchAsYouTypeEnabled) { this.updateUrlNoReload(); this.reloadStudyList(); }
      }
    },

    isFilteringOnlyOnLabels() {
      let hasGeneric = false;
      for (const tag of this.uiOptions.StudyListColumns) {
        if (!["StudyDate", "PatientBirthDate", "modalities", "seriesCount", "instancesCount", "seriesAndInstancesCount"].includes(tag)) {
          if (this.filterGenericTags[tag]) hasGeneric = true;
        }
      }
      return !this.filterStudyDate && !this.filterPatientBirthDate && !hasGeneric && this.filterLabels.length > 0 && !this.filterOrderBy.length;
    },

    async updateUrlNoReload() {
      this.updatingRouteWithoutReload = true;
      await this.updateUrl();
      this.updatingRouteWithoutReload = false;
    },

    async updateUrl() {
      const query = {};
      if (this.sourceType !== SourceType.LOCAL_ORTHANC) {
        query["source-type"] = this.sourceType === SourceType.REMOTE_DICOM ? "dicom" : "dicom-web";
        query["remote-source"] = this.remoteSource;
      }
      if (this.clipFilter("StudyDate", this.filterStudyDate)) query.StudyDate = this.filterStudyDate;
      if (this.clipFilter("PatientBirthDate", this.filterPatientBirthDate)) query.PatientBirthDate = this.filterPatientBirthDate;
      const modFilter = this.getModalityFilter();
      if (modFilter) query.ModalitiesInStudy = modFilter;
      for (const [k, v] of Object.entries(this.filterGenericTags)) {
        if (this.clipFilter(k, v)) query[k] = v;
      }
      if (this.filterLabels.length > 0) {
        query.labels = this.filterLabels.join(",");
        if (this.multiLabelsFilterLabelsConstraint !== "All") query["labels-constraint"] = this.multiLabelsFilterLabelsConstraint;
      }
      if (this.filterOrderBy.length > 0 && this.sourceType === SourceType.LOCAL_ORTHANC) {
        query["order-by"] = this.filterOrderBy.map(o => `${o.Type},${o.Key},${o.Direction}`).join(";");
      }
      await this.$router.replace("/filtered-studies?" + new URLSearchParams(query).toString());
    },

    async updateFilterFromRoute(filters) {
      this.updatingFilterUi = true;
      await this.$store.dispatch("studies/clearStudies");
      await this.$store.dispatch("studies/clearFilterNoReload");
      const kv = {};
      if ("source-type" in filters && "remote-source" in filters) {
        this.sourceType = filters["source-type"].toLowerCase() === "dicom" ? SourceType.REMOTE_DICOM : SourceType.REMOTE_DICOM_WEB;
        this.remoteSource = filters["remote-source"];
      } else {
        this.sourceType = SourceType.LOCAL_ORTHANC;
        this.remoteSource = null;
      }
      await this.$store.dispatch("studies/updateSource", { "source-type": this.sourceType, "remote-source": this.remoteSource });
      let routeHasOrderBy = false;
      const labelsConstraint = filters["labels-constraint"] || "All";
      const hasWithoutLabels = Object.keys(filters).includes("without-labels");
      for (const [fk, fv] of Object.entries(filters)) {
        if (fk === "labels" && !hasWithoutLabels) {
          const labels = fv.split(",");
          kv[fk] = labels;
          await this.$store.dispatch("studies/updateLabelFilterNoReload", { labels, constraint: labelsConstraint });
        } else if (fk === "labels-constraint" && !hasWithoutLabels) {
          this.multiLabelsFilterLabelsConstraint = fv;
        } else if (fk === "order-by") {
          if (this.sourceType === SourceType.LOCAL_ORTHANC) { this.updateOrderBy(fv, false); routeHasOrderBy = true; }
        } else if (fk[0] === fk[0].toUpperCase()) {
          kv[fk] = fv;
          await this.$store.dispatch("studies/updateFilterNoReload", { dicomTagName: fk, value: fv });
        }
      }
      if (hasWithoutLabels) {
        await this.$store.dispatch("studies/updateLabelFilterNoReload", { labels: [], constraint: "None" });
        this.showStudiesWithoutLabels = true;
      } else {
        this.showStudiesWithoutLabels = false;
      }
      if (!routeHasOrderBy && this.uiOptions.DefaultOrdering) this.updateOrderBy(this.uiOptions.DefaultOrdering, false);
      this.updateFilterForm(kv, labelsConstraint, this.showStudiesWithoutLabels);
      if (this.sourceType === SourceType.LOCAL_ORTHANC || !this["studies/isFilterEmpty"]) await this.reloadStudyList();
      this.multiLabelsComponentKey++;
      await nextTick();
      this.updatingFilterUi = false;
    },

    async updateFilterForm(filters, labelsConstraint, showStudiesWithoutLabels) {
      this.emptyFilterForm();
      this.multiLabelsFilterLabelsConstraint = labelsConstraint;
      this.showStudiesWithoutLabels = showStudiesWithoutLabels;
      for (const [key, value] of Object.entries(filters)) {
        if (key === "labels") {
          this.filterLabels = value;
        } else if (key === "StudyDate") {
          this.filterStudyDate = value;
          this.filterStudyDateForDatePicker = dateHelpers.parseDateForDatePicker(value);
        } else if (key === "PatientBirthDate") {
          this.filterPatientBirthDate = value;
          this.filterPatientBirthDateForDatePicker = dateHelpers.parseDateForDatePicker(value);
        } else if (key === "ModalitiesInStudy") {
          const mods = value.split("\\");
          if (mods.length > 0) {
            let all = true, none = true;
            for (const m of this.uiOptions.ModalitiesFilter) {
              const isIn = mods.includes(m);
              this.filterModalities[m] = isIn;
              all &= isIn; none &= !isIn;
            }
            this.allModalities = all; this.noneModalities = none;
          }
        } else if (key !== "order-by") {
          this.filterGenericTags[key] = value;
        }
      }
      await nextTick();
    },

    async extendStudyList() {
      if (this.sourceType === SourceType.LOCAL_ORTHANC && this.hasExtendedFind) {
        await this.$store.dispatch("studies/extendFilteredStudies");
      }
    },

    async reloadStudyList() {
      if (this.sourceType === SourceType.LOCAL_ORTHANC && this.hasExtendedFind) {
        if (this.uiOptions.StudyListContentIfNoSearch === "empty") {
          this.status = Status.UNDEFINED;
          await this.$store.dispatch("studies/clearStudies");
          if (!this["studies/isFilterEmpty"]) await this.$store.dispatch("studies/reloadFilteredStudies");
        } else {
          if (this["studies/isMostRecentOrdering"]) this.status = Status.LOADING_MOST_RECENT_STUDIES;
          await this.$store.dispatch("studies/clearStudies");
          await this.$store.dispatch("studies/reloadFilteredStudies");
          this.status = this["studies/isMostRecentOrdering"] ? Status.DISPLAYING_MOST_RECENT_STUDIES : Status.DISPLAYING_FILTERED_STUDIES;
        }
      } else {
        const shouldShowMostRecentsWithLabel = this.uiOptions.StudyListContentIfNoSearch === "most-recents" && this.isFilteringOnlyOnLabels();
        if (this.sourceType === SourceType.LOCAL_ORTHANC && (this["studies/isFilterEmpty"] || shouldShowMostRecentsWithLabel)) {
          await this.$store.dispatch("studies/clearStudies");
          if (this.uiOptions.StudyListContentIfNoSearch === "empty") return;
          if (this.uiOptions.StudyListContentIfNoSearch === "most-recents" && this.hasExtendedFind) {
            const studies = await api.getMostRecentStudiesExtended(this.filterLabels.length > 0 ? this.filterLabels[0] : null);
            for (const study of studies) this.$store.dispatch("studies/addStudy", { studyId: study.ID, study, reloadStats: false });
          } else if (this.uiOptions.StudyListContentIfNoSearch === "most-recents") {
            if (this.status === Status.LOADING_MOST_RECENT_STUDIES) { this.shouldStopLoadingMostRecentStudies = true; this.status = Status.DISPLAYING_MOST_RECENT_STUDIES; }
            const lastChangeId = await api.getLastChangeId();
            await this.$store.dispatch("studies/clearStudies");
            this.mostRecentStudiesIds = new Set();
            this.shouldStopLoadingMostRecentStudies = false;
            this.status = Status.LOADING_MOST_RECENT_STUDIES;
            this.loadStudiesFromChange(lastChangeId, 1000);
          }
        } else {
          this.shouldStopLoadingMostRecentStudies = true;
          this.status = Status.UNDEFINED;
          await this.$store.dispatch("studies/reloadFilteredStudies");
        }
      }
    },

    async loadStudiesFromChange(toChangeId, limit) {
      let changes, changesResponse;
      if (this.hasExtendedChanges) {
        changesResponse = await api.getChangesExtended(toChangeId, limit, ["NewStudy", "StableStudy"]);
        changes = changesResponse.Changes;
      } else {
        changesResponse = await api.getChanges(toChangeId - limit, limit);
        changes = changesResponse.Changes.reverse();
      }
      for (const change of changes) {
        if ((change.ChangeType === "NewStudy" || change.ChangeType === "StableStudy") && !this.mostRecentStudiesIds.has(change.ID)) {
          if (this.shouldStopLoadingMostRecentStudies) return;
          try {
            const study = await api.getStudy(change.ID);
            if (this.filterLabels.length === 0 || this.filterLabels.some(l => study.Labels.includes(l))) {
              this.$store.dispatch("studies/addStudy", { studyId: change.ID, study, reloadStats: false });
            }
            this.mostRecentStudiesIds.add(change.ID);
            if (this.mostRecentStudiesIds.size === this.uiOptions.MaxStudiesDisplayed) { this.status = Status.DISPLAYING_MOST_RECENT_STUDIES; return; }
          } catch { console.warn("Unable to load study - not authorized ?"); }
        }
      }
      if (!this.shouldStopLoadingMostRecentStudies) {
        if (this.mostRecentStudiesIds.size < this.statistics.CountStudies) {
          if (this.hasExtendedChanges) { if (!changesResponse.Done) setTimeout(() => this.loadStudiesFromChange(changesResponse.First, 1000), 1); }
          else { if (toChangeId !== changesResponse.First) setTimeout(() => this.loadStudiesFromChange(Math.max(0, toChangeId - 1000), 1000), 1); }
        } else { this.status = Status.DISPLAYING_MOST_RECENT_STUDIES; }
      } else { this.status = Status.DISPLAYING_MOST_RECENT_STUDIES; }
    },

    onDeletedStudy(studyId) { this.$store.dispatch("studies/deleteStudy", { studyId }); },

    visibilityChanged(isVisible, entry) {
      if (!isVisible) return;
      const studyId = entry.target.id;
      const ids = this.isCardView ? this.sortedStudiesForCards : this.studiesIds;
      if (studyId === ids[ids.length - 1]) this.extendStudyList();
    },

    async syncFromMobileFilters(appliedFilters) {
      for (const tag of ["PatientName", "PatientID", "AccessionNumber", "StudyDescription"]) {
        this.filterGenericTags[tag] = appliedFilters[tag] || "";
      }

      if (appliedFilters.StudyDate) {
        this.filterStudyDate = appliedFilters.StudyDate;
        this.filterStudyDateForDatePicker = dateHelpers.parseDateForDatePicker(appliedFilters.StudyDate);
      } else {
        this.filterStudyDate = "";
        this.filterStudyDateForDatePicker = null;
      }

      if (appliedFilters.PatientBirthDate) {
        this.filterPatientBirthDate = appliedFilters.PatientBirthDate;
        this.filterPatientBirthDateForDatePicker = dateHelpers.parseDateForDatePicker(appliedFilters.PatientBirthDate);
      } else {
        this.filterPatientBirthDate = "";
        this.filterPatientBirthDateForDatePicker = null;
      }

      if (appliedFilters.ModalitiesInStudy) {
        if (appliedFilters.ModalitiesInStudy === "NONE") {
          for (const mod of this.uiOptions.ModalitiesFilter) this.filterModalities[mod] = false;
          this.allModalities = false;
          this.noneModalities = true;
          this.activeModalityFilter = null;
        } else {
          const selected = appliedFilters.ModalitiesInStudy.split("\\");
          this.activeModalityFilter = selected.join(", ");
          for (const mod of this.uiOptions.ModalitiesFilter) {
            this.filterModalities[mod] = selected.includes(mod);
          }
          this.allModalities = false;
          this.noneModalities = false;
        }
      } else {
        for (const mod of this.uiOptions.ModalitiesFilter) this.filterModalities[mod] = true;
        this.allModalities = true;
        this.noneModalities = false;
        this.activeModalityFilter = null;
      }

      this.filterLabels = appliedFilters.labels || [];

      await nextTick();
    }
  },

  components: { StudyItem, StudyCard, ResourceButtonGroup, StudyListFilters, StudyListAlerts, Toasts },
};
</script>

<template>
  <div>
    <!-- Remote browsing warning -->
    <div v-if="isRemoteDicom || isRemoteDicomWeb" class="remote-browsing-warning">
      <div>
        <p v-if="isRemoteDicom" v-html="$t('remote_dicom_browsing', { source: remoteSource })"></p>
        <p v-if="isRemoteDicomWeb" v-html="$t('remote_dicom_web_browsing', { source: remoteSource })"></p>
      </div>
    </div>

    <table class="table table-sm study-table table-borderless">
      <!-- ═══════════════════════════════════════════════════════════
           THEAD — делегирован StudyListFilters
           (колонки, инпуты фильтров, labels; quick-filters через Teleport)
           ═══════════════════════════════════════════════════════════ -->
      <StudyListFilters :isTableView="isTableView" :isCardView="isCardView" :columns="columns"
        :widthColum1="widthColum1" :colSpanClearFilter="colSpanClearFilter" :hasPrimaryViewerIcon="hasPrimaryViewerIcon"
        :hasQuickActionsColumn="hasQuickActionsColumn" :hasPdfReportIcon="hasPdfReportIcon"
        :isMultiLabelsFilterVisible="isMultiLabelsFilterVisible"
        :colSpanBeforeMultiLabelsFilter="colSpanBeforeMultiLabelsFilter"
        :colSpanMultiLabelsFilter="colSpanMultiLabelsFilter"
        :colSpanAfterMultiLabelsFilter="colSpanAfterMultiLabelsFilter" :sourceType="sourceType"
        :currentOrderByTag="currentOrderByTag" :currentOrderDirection="currentOrderDirection"
        :filterStudyDateForDatePicker="filterStudyDateForDatePicker"
        @update:filterStudyDateForDatePicker="filterStudyDateForDatePicker = $event"
        :filterPatientBirthDateForDatePicker="filterPatientBirthDateForDatePicker"
        @update:filterPatientBirthDateForDatePicker="filterPatientBirthDateForDatePicker = $event"
        :filterGenericTags="filterGenericTags" @update:filterGenericTags="filterGenericTags = $event"
        :filterModalities="filterModalities" @update:filterModalities="filterModalities = $event"
        :allModalities="allModalities" :noneModalities="noneModalities" :datePickerPresetRanges="datePickerPresetRanges"
        :isDarkMode="isDarkMode" :filterLabels="filterLabels"
        :multiLabelsFilterLabelsConstraint="multiLabelsFilterLabelsConstraint"
        @update:multiLabelsFilterLabelsConstraint="multiLabelsFilterLabelsConstraint = $event"
        :multiLabelsComponentKey="multiLabelsComponentKey" :filterStudyDate="filterStudyDate"
        :cardViewSort="cardViewSort" @update:cardViewSort="cardViewSort = $event"
        :cardViewSortOptions="cardViewSortOptions" :activeModalityFilter="activeModalityFilter"
        :isSearchButtonEnabled="isSearchButtonEnabled" :isSearching="isSearching" @search="search"
        @clear-filters="clearFilters" @toggle-order="handleToggleOrder"
        @toggle-modality-filter="handleToggleModalityFilter" @labels-updated="onMultiLabelsFilterChanged"
        @clear-active-modality-filter="clearActiveModalityFilter" @set-card-quick-filter="setCardQuickFilter" />
      <!-- ═══════════════════════════════════════════════════════════
           ACTIONS ROW
           ═══════════════════════════════════════════════════════════ -->
      <tr class="study-table-actions">
        <!-- Checkbox -->
        <th class="actions-fixed-left" width="5%" colspan="1" scope="col">
          <div class="form-check">
            <input class="form-check-input" type="checkbox" v-model="allSelected" :indeterminate="isPartialSelected"
              @click="clickSelectAll" />
            <span class="selected-count">{{ selectedStudiesCount }}</span>
          </div>
        </th>

        <!-- Центральная адаптивная область -->
        <th class="actions-adaptive-center" width="90%"
          :colspan="colSpanMultiLabelsFilter + colSpanAfterMultiLabelsFilter" scope="col">
          <div class="adaptive-controls-wrapper">
            <div id="study-quick-filters-slot" class="center-left-group"></div>

            <div class="action-buttons-group">
              <ResourceButtonGroup :resourceLevel="'bulk'" smallIcons="true" />
            </div>

            <!-- Alerts — делегированы StudyListAlerts -->
            <StudyListAlerts :isLoadingMostRecentStudies="isLoadingMostRecentStudies"
              :isDisplayingMostRecentStudies="isDisplayingMostRecentStudies"
              :notShowingAllResults="notShowingAllResults" :showEmptyStudyListIfNoSearch="showEmptyStudyListIfNoSearch"
              :isStudyListEmpty="isStudyListEmpty" :filterGenericTags="filterGenericTags"
              :filterStudyDate="filterStudyDate" :filterPatientBirthDate="filterPatientBirthDate"
              :filterLabels="filterLabels" :activeModalityFilter="activeModalityFilter" :isCardView="isCardView"
              @clear-text-filter="clearTextFilter" @clear-birthdate-filter="clearBirthDateFilter"
              @clear-labels-filter="clearLabelsFilter" @clear-all-filters="clearFilters"
              @clear-study-date-filter="clearStudyDateFilter" />

            <!-- Слот для search button (заполняется через Teleport из StudyListFilters) -->
            <div id="study-search-slot"></div>
          </div>
        </th>

        <!-- View mode toggle -->
        <th class="actions-fixed-right" width="2%" colspan="2" scope="col">
          <div class="view-mode-toggle">
            <button class="view-mode-btn" :class="{ active: isTableView }" @click="setViewMode('table')"
              :title="$t('table_view')">
              <i class="bi bi-table"></i>
            </button>
            <button class="view-mode-btn" :class="{ active: isCardView }" @click="setViewMode('cards')"
              :title="$t('card_view')">
              <i class="bi bi-grid-3x3-gap-fill"></i>
            </button>
          </div>
        </th>
      </tr>

      <!-- TABLE VIEW -->
      <StudyItem v-if="isTableView" v-for="(studyId, index) in studiesIds" :key="studyId" :id="studyId"
        :studyId="studyId" v-observe-visibility="index === studiesIds.length - 1 ? {
          callback: visibilityChanged,
          intersection: { threshold: 0.1 },
          once: false
        } : false" @deletedStudy="onDeletedStudy" @filter-modality="onFilterModality" />
    </table>

    <!-- CARDS VIEW -->
    <div v-if="isCardView" class="studies-cards-container">
      <div class="studies-cards-grid">
        <StudyCard v-for="(studyId, index) in sortedStudiesForCards" :key="studyId + '-' + cardViewSort" :id="studyId"
          :studyId="studyId" v-observe-visibility="index === sortedStudiesForCards.length - 1 ? {
            callback: visibilityChanged,
            intersection: { threshold: 0.1 },
            once: false
          } : false" @deletedStudy="onDeletedStudy" @filter-modality="onFilterModality" />
      </div>
    </div>

    <!-- MOBILE FILTERS MODAL -->

    <Toasts />
  </div>
</template>

<style>
/* =============================================================================
   STUDYLIST.VUE - GLOBAL STYLES
   Все стили остаются глобальными, т.к. на них полагаются дочерние компоненты.
   ============================================================================= */

:root {
  --filter-margin: 6px;
  --filter-padding: 8px;
  --study-border-radius: 8px;
  --study-transition: all 0.2s ease;
  --study-hover-shadow: 0 2px 8px rgba(59, 130, 246, 0.15);
  --study-active-shadow: 0 4px 12px rgba(59, 130, 246, 0.25);
}

/* Remote Warning */
.remote-browsing-warning {
  background: linear-gradient(135deg, #3b82f6 0%, #8b5cf6 100%);
  text-align: center;
  font-weight: 600;
  height: 2.5rem;
  line-height: 2.5rem;
  color: white;
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
  font-family: "Segoe UI", system-ui, -apple-system, sans-serif;
  letter-spacing: 0.5px;
}

.remote-browsing-warning p {
  margin: 0;
  padding: 0;
}

/* Study Table */
.study-table {
  table-layout: fixed;
  font-family: "Segoe UI", system-ui, -apple-system, sans-serif;
  font-size: 13px;
  border-collapse: separate;
  border-spacing: 0;
}

/* Column Titles */
.study-column-titles {
  background: linear-gradient(180deg, #f8fafc 0%, #f1f5f9 100%) !important;
  border-bottom: 2px solid #e2e8f0;
  font-size: 12px;
  font-weight: 600;
  color: #475569;
  letter-spacing: 0.3px;
  text-transform: uppercase;
}

.study-table-title {
  text-align: left;
  padding: 0 4px !important;
  vertical-align: middle;
  position: relative;
  top: 6px;
  left: 4px;
  transition: var(--study-transition);
}

.study-table-title:hover {
  background-color: rgba(59, 130, 246, 0.05);
}

.title-container {
  position: relative;
  width: 100%;
  height: 2rem;
  display: flex;
  align-items: center;
}

.title-text {
  flex: 1;
  padding: 0 4px;
  border-right: 1px solid #e2e8f0;
  text-overflow: ellipsis;
  overflow: hidden;
  white-space: nowrap;
  transition: var(--study-transition);
}

.is-orderable {
  cursor: pointer;
  user-select: none;
  color: #3b82f6;
  font-weight: 600;
}

.is-orderable:hover {
  color: #2563eb;
  transform: translateY(-1px);
}

.title-arrow {
  position: absolute;
  right: 4px;
  bottom: 2px;
  font-size: 16px;
  color: #3b82f6;
  animation: arrowPulse 1.5s ease-in-out infinite;
}

@keyframes arrowPulse {

  0%,
  100% {
    opacity: 1;
  }

  50% {
    opacity: 0.6;
  }
}

/* Filters Row */
.study-table-filters {
  background: linear-gradient(180deg, #fff 0%, #f8fafc 100%);
  border-bottom: 1px solid #e2e8f0;
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.02);
}

.study-table-filters>th {
  background-color: transparent;
  padding: 8px 6px !important;
  vertical-align: middle;
}

input.form-control.study-list-filter {
  margin-top: var(--filter-margin);
  margin-bottom: var(--filter-margin);
  padding: var(--filter-padding);
  border: 1px solid #cbd5e1;
  border-radius: 6px;
  font-size: 13px;
  transition: var(--study-transition);
  background: #fff;
}

input.form-control.study-list-filter:focus {
  border-color: #3b82f6;
  box-shadow: 0 0 0 3px rgba(59, 130, 246, 0.1);
  outline: none;
}

input.form-control.study-list-filter::placeholder {
  color: #94a3b8;
  font-style: italic;
}

button.form-control.study-list-filter,
.filter-button {
  margin-top: var(--filter-margin);
  margin-bottom: var(--filter-margin);
  padding: var(--filter-padding);
  border: 1px solid #cbd5e1 !important;
  border-radius: 6px;
  background: #fff;
  transition: var(--study-transition);
  font-weight: 500;
}

.filter-button:hover {
  background: #f1f5f9;
  border-color: #3b82f6 !important;
  transform: translateY(-1px);
  box-shadow: 0 2px 4px rgba(59, 130, 246, 0.15);
}

button.form-control.study-list-filter.btn.filter-button.btn-sm {
  max-width: 36px;
}

button.form-control.study-list-filter.btn.filter-button {
  color: white;
  border: none !important;
  background: #9ca3a9;
}

button.form-control.study-list-filter.btn.filter-button:hover {
  color: #dc1529;
  font-weight: 500;
  border: none !important;
  transform: scale(1.1);
}

.is-invalid-filter {
  border-color: #ef4444 !important;
  box-shadow: 0 0 0 3px rgba(239, 68, 68, 0.15) !important;
  animation: shake 0.3s;
}

@keyframes shake {

  0%,
  100% {
    transform: translateX(0);
  }

  25% {
    transform: translateX(-4px);
  }

  75% {
    transform: translateX(4px);
  }
}

/* Search Button */
.search-button {
  padding: 6px 12px;
  font-weight: 600;
  display: flex;
  align-items: center;
  justify-content: center;
  min-width: 36px;
  height: 36px;
  border: none;
  border-radius: 6px;
  cursor: pointer;
  transition: all 0.2s ease;
}

.search-button.is-not-searching {
  background: linear-gradient(135deg, #3b82f6 0%, #2563eb 100%) !important;
  color: white;
}

.search-button.is-not-searching:hover {
  background: linear-gradient(135deg, #2563eb 0%, #1d4ed8 100%) !important;
  box-shadow: var(--study-hover-shadow);
}

.search-button.is-searching {
  background: linear-gradient(135deg, #8b5cf6 0%, #7c3aed 100%) !important;
  color: white;
}

.search-button-wrapper {
  flex-shrink: 0;
  margin-left: auto;
}

/* Actions Row */
.study-table-actions {
  display: table-row !important;
  width: 100%;
}

.study-table-actions>th {
  background: linear-gradient(180deg, #f8fafc 0%, #f1f5f9 100%) !important;
  border-bottom: 2px solid #e2e8f0;
  vertical-align: middle;
  padding: 8px !important;
}

.study-table-actions>th>div {
  background-color: transparent !important;
}



/* Checkbox */
.actions-fixed-left {
  width: 80px;
  min-width: 80px;
  white-space: nowrap;
}

.actions-fixed-left .form-check {
  display: flex;
  align-items: center;
  gap: 6px;
  margin: 0;
  margin-left: 1.35vw;
}

.actions-fixed-left .selected-count {
  font-weight: 400;
  font-size: small;
}

.study-table-actions .form-check {
  margin-left: 1.45vw;
  display: flex;
  align-items: center;
  justify-content: flex-start;
  gap: 8px;
}

.study-table-actions .form-check-input {
  width: 18px;
  height: 18px;
  border: 2px solid #cbd5e1;
  border-radius: 4px;
  cursor: pointer;
  transition: var(--study-transition);
}

.study-table-actions .form-check-input:checked {
  background-color: #3b82f6;
  border-color: #3b82f6;
}

.study-table-actions .form-check-input:hover {
  border-color: #3b82f6;
  box-shadow: 0 0 0 3px rgba(59, 130, 246, 0.1);
}

.form-check:hover {
  transform: scale(1.1);
}

/* Adaptive Controls */
.actions-adaptive-center {
  width: 100%;
}

.adaptive-controls-wrapper {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  width: 100%;
}

.center-left-group {
  display: flex;
  align-items: center;
  gap: 8px;
  flex-shrink: 0;
}

.action-buttons-group {
  display: flex;
  align-items: center;
  flex-shrink: 0;
  margin-right: auto;
}

.action-buttons-group :deep(.btn-group) {
  display: flex;
  margin: 0;
  padding: 0;
  gap: 0;
}

.action-buttons-group :deep(.btn) {
  margin: 0 !important;
  border-radius: 0 !important;
  border-right-width: 0 !important;
}

.action-buttons-group :deep(.btn:first-child) {
  border-top-left-radius: 6px !important;
  border-bottom-left-radius: 6px !important;
}

.action-buttons-group :deep(.btn:last-child) {
  border-top-right-radius: 6px !important;
  border-bottom-right-radius: 6px !important;
  border-right-width: 1px !important;
}

/* View Mode Toggle */
.actions-fixed-right {
  width: 80px;
  min-width: 80px;
  white-space: nowrap;
}

.view-mode-toggle {
  display: flex;
  gap: 4px;
  justify-content: flex-end;
  padding: 4px;
  height: 32px;
}

.view-mode-btn {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 32px;
  height: 32px;
  padding: 0;
  border: none;
  border-radius: 6px;
  background: transparent;
  color: #64748b;
  cursor: pointer;
  transition: all 0.2s ease;
}

.view-mode-btn:hover {
  background: #f1f5f9;
  color: #3b82f6;
  transform: scale(1.1);
}

.view-mode-btn.active {
  background: linear-gradient(135deg, #3b82f6 0%, #2563eb 100%);
  color: white;
  box-shadow: 0 2px 8px rgba(59, 130, 246, 0.3);
}

.btn-secondary.btn-icon-small:hover {
  transform: scale(1.05);
}

/* Quick Filters */
.quick-filters {
  display: flex;
  gap: 4px;
  flex-shrink: 0;
}

.quick-filter-btn {
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 6px 10px;
  border: 1px solid #cbd5e1;
  border-radius: 6px;
  background: white;
  color: #64748b;
  font-size: 12px;
  cursor: pointer;
  transition: all 0.2s ease;
  line-height: 1;
  flex-shrink: 0;
}

.quick-filter-btn:hover {
  border-color: #3b82f6;
  color: #3b82f6;
  background: rgba(59, 130, 246, 0.05);
}

.quick-filter-btn.active {
  border-color: #3b82f6;
  background: linear-gradient(135deg, #3b82f6 0%, #2563eb 100%);
  color: white;
}

.card-sort-dropdown {
  flex-shrink: 0;
}

.card-sort-dropdown select {
  width: 40px;
  padding: 6px 8px;
  font-size: 10px;
  background: #fff;
  border: 1px solid #cbd5e1;
  border-radius: 6px;
  cursor: pointer;
}

.card-sort-dropdown select:hover {
  border-color: #3b82f6;
  color: #3b82f6;
  background: #3b82f60d;
}

.active-modality-filter .badge {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  font-size: 12px;
  padding: 6px 10px;
  cursor: default;
  white-space: nowrap;
}

.active-modality-filter .bi-x-circle {
  cursor: pointer;
  font-size: 14px;
}

.active-modality-filter .bi-x-circle:hover {
  opacity: 0.8;
}

/* Study Rows */
.study-table> :nth-child(odd)>tr>td {
  background-color: #fff;
  min-width: 100%;
}

tr.study-row-expanded {
  background: white !important;
}

.study-table> :nth-child(even)>tr>td {
  background-color: #f8fafc;
}

.study-table>tbody>tr:first-child:hover>* {
  background: linear-gradient(90deg, rgba(59, 130, 246, 0.08) 0%, rgba(59, 130, 246, 0.03) 100%);
  cursor: pointer;
}

.study-table td {
  text-align: left;
  padding: 12px 10px;
  border-bottom: 1px solid #f1f5f9;
  transition: var(--study-transition);
  font-size: 13px;
  color: #334155;
}

.study-table> :last-child {
  border-bottom: 2px solid #e2e8f0;
}

/* Study Details Table */
.study-details-table {
  margin-top: var(--details-top-margin);
  width: 97% !important;
  font-size: 0.9rem;
  border-radius: var(--study-border-radius);
  overflow: hidden;
  box-shadow: 0 4px 6px rgba(0, 0, 0, 0.05);
}

.study-details-table> :not(caption)>*>*,
.study-details-table>*>* {
  background: linear-gradient(135deg, #f8fafc 0%, #f1f5f9 100%) !important;
  border: 1px solid #e2e8f0;
}

.study-details-table td {
  vertical-align: top;
  padding: 10px;
  color: #475569;
}

.study-details-table th {
  font-weight: 600;
  color: #334155;
  background: linear-gradient(135deg, #e2e8f0 0%, #cbd5e1 100%) !important;
}

/* Modality Filter Dropdown */
#modality-filter-dropdown {
  border-radius: 8px;
  box-shadow: 0 8px 16px rgba(0, 0, 0, 0.15);
  border: 1px solid #e2e8f0;
  padding: 8px 0;
  min-width: 200px;
}

#modality-filter-dropdown .dropdown-item {
  padding: 8px 16px;
  font-size: 13px;
  color: #475569;
  transition: var(--study-transition);
  cursor: pointer;
}

#modality-filter-dropdown .dropdown-item:hover {
  background: linear-gradient(90deg, rgba(59, 130, 246, 0.1) 0%, rgba(59, 130, 246, 0.05) 100%);
  color: #3b82f6;
}

#modality-filter-dropdown .dropdown-item input[type="checkbox"] {
  width: 16px;
  height: 16px;
  margin-right: 8px;
  cursor: pointer;
  accent-color: #3b82f6;
}

#modality-filter-dropdown .dropdown-divider {
  margin: 8px 0;
  border-color: #e2e8f0;
}

#modality-filter-dropdown button.btn-primary {
  margin: 8px auto;
  width: calc(100% - 40px);
  background: linear-gradient(135deg, #3b82f6 0%, #2563eb 100%);
  border: none;
  padding: 8px;
  border-radius: 6px;
  font-weight: 600;
}

#modality-filter-dropdown button.btn-primary:hover {
  background: linear-gradient(135deg, #2563eb 0%, #1d4ed8 100%);
  box-shadow: 0 4px 8px rgba(59, 130, 246, 0.3);
  transform: translateY(-1px);
}

/* Labels */
.study-table-actions th label.form-check-label {
  font-size: 12px;
  font-weight: 500;
  color: #475569;
  cursor: pointer;
  transition: var(--study-transition);
}

.study-table-actions th label.form-check-label:hover {
  color: #3b82f6;
}

.study-table-actions th input[type="radio"] {
  accent-color: #3b82f6;
  cursor: pointer;
  width: 16px;
  height: 16px;
}

/* Cards Container */
.studies-cards-container {
  padding: 20px;
  background: linear-gradient(180deg, #f8fafc 0%, #fff 100%);
  min-height: 400px;
}

.studies-cards-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(320px, 1fr));
  gap: 20px;
  animation: fadeIn 0.4s ease;
  grid-auto-flow: dense;
  transition: .5s;
}

.studies-cards-grid>.study-card.expanded {
  grid-column: 1 / -1 !important;
}

.studies-cards-grid>.study-card {
  transition: all 0.3s ease;
}

@keyframes fadeIn {
  from {
    opacity: 0;
    transform: translateY(10px);
  }

  to {
    opacity: 1;
    transform: translateY(0);
  }
}

/* Cards - Details fix */
.studies-cards-grid .study-card .details-content {
  width: 100%;
  min-width: 0;
  overflow-x: auto;
}

.studies-cards-grid .study-card .details-content table {
  width: 100%;
  table-layout: auto;
}

.studies-cards-grid .study-card .details-content td,
.studies-cards-grid .study-card .details-content th {
  white-space: normal;
  word-wrap: break-word;
}

.studies-cards-grid .study-card .details-content>div {
  width: 100%;
}

.studies-cards-grid .study-details-table {
  margin-left: 0 !important;
  width: 100% !important;
}

.studies-cards-grid .study-details-table td,
.studies-cards-grid .study-details-table th {
  padding: 8px 12px;
  text-align: left;
  vertical-align: top;
  white-space: normal;
}

.studies-cards-grid .series-table {
  width: 100%;
  table-layout: auto;
}

.studies-cards-grid .series-table td,
.studies-cards-grid .series-table th {
  white-space: nowrap;
  padding: 6px 10px;
}

.studies-cards-container .dropdown,
.studies-cards-container .btn-group {
  position: static !important;
}

.studies-cards-container .dropdown-menu {
  position: absolute !important;
  z-index: 9999 !important;
}

/* Spinner */
.spinner-border-sm {
  animation: spinner 0.75s linear infinite;
}

@keyframes spinner {
  to {
    transform: rotate(360deg);
  }
}

/* Scrollbar */
.study-table::-webkit-scrollbar {
  width: 8px;
  height: 8px;
}

.study-table::-webkit-scrollbar-track {
  background: #f1f5f9;
  border-radius: 4px;
}

.study-table::-webkit-scrollbar-thumb {
  background: linear-gradient(180deg, #94a3b8 0%, #64748b 100%);
  border-radius: 4px;
}

.study-table::-webkit-scrollbar-thumb:hover {
  background: linear-gradient(180deg, #64748b 0%, #475569 100%);
}

/* Focus */
.study-table button:focus-visible,
.study-table input:focus-visible {
  outline: 2px solid #3b82f6;
  outline-offset: 2px;
}

/* ─── Responsive ─── */
@media (max-width: 1200px) {
  .study-table {
    font-size: 12px;
  }

  .study-table td {
    padding: 8px 6px;
  }

  .adaptive-controls-wrapper {
    gap: 8px;
  }

  .center-left-group {
    gap: 6px;
  }

  .quick-filter-btn {
    padding: 5px 8px;
  }
}

@media (max-width: 992px) {
  .center-left-group {
    flex-wrap: wrap;
    max-width: 180px;
  }

  .quick-filters {
    flex-wrap: wrap;
  }
}

@media (max-width: 768px) {
  .study-table-actions>th {
    width: 100% !important;
    box-sizing: border-box;
  }

  .quick-filter-btn i {
    font-size: 24px;
  }

  .actions-fixed-left {
    display: none !important;
    width: 50% !important;
  }

  .actions-fixed-right {
    display: inline-block !important;
    width: 50% !important;
    text-align: right;
  }

  .actions-adaptive-center {
    border-top: 1px solid #e2e8f0;
  }

  .adaptive-controls-wrapper {
    flex-wrap: wrap;
    justify-content: center;
  }

  .center-left-group {
    width: 100%;
    max-width: none;
    justify-content: center;
    margin-bottom: 8px;
    order: 0;
  }

  .action-buttons-group {
    order: 1;
    margin-right: 0;
  }

  .search-button-wrapper {
    order: 2;
    margin-left: 0;
  }

  .studies-cards-grid {
    grid-template-columns: repeat(auto-fill, minmax(200px, 1fr));
    gap: 8px;
  }
}

@media (max-width: 480px) {
  .quick-filters {
    display: flex;
    width: auto;
  }

  .quick-filter-btn {
    padding: 4px 6px;
    font-size: 11px;
  }

  .card-sort-dropdown select {
    padding: 4px 6px;
    font-size: 16px;
    height: 40px;
  }

  .studies-cards-grid {
    grid-template-columns: 1fr;
    gap: 12px;
  }
}

/* Mobile portrait */
@media (max-width: 768px) and (orientation: portrait) {
  tr.study-table-actions {
    display: flex !important;
    flex-wrap: wrap;
    flex-direction: row;
    align-content: center;
    justify-content: center;
    align-items: flex-start;
    width: 100vw;
  }

  thead.sticky-top {
    display: flex;
    flex-direction: row;
    flex-wrap: wrap;
    align-content: center;
    justify-content: center;
  }

  tr.study-column-titles {
    display: none !important;
  }

  tr.study-table-filters {
    display: none !important;
  }

  th.actions-fixed-right {
    display: none !important;
  }

  .studies-cards-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(200px, 1fr));
    grid-auto-flow: dense;
    gap: 8px;
    padding: 8px;
  }

  .studies-cards-container {
    padding: 8px;
  }

  .actions-adaptive-center {
    width: 100% !important;
  }

  .adaptive-controls-wrapper {
    flex-direction: column;
    gap: 8px;
    padding: 8px 0;
  }

  .center-left-group {
    width: 100%;
    justify-content: space-between;
    flex-wrap: wrap;
  }

  .action-buttons-group {
    margin: 0 auto;
  }

  .search-button-wrapper {
    margin: 0 auto;
  }

  .quick-filters {
    gap: 4px;
  }

  .quick-filter-btn {
    padding: 6px 8px;
    font-size: 12px;
  }

  .actions-fixed-left {
    width: auto !important;
    min-width: auto !important;
  }

  .actions-fixed-left .form-check {
    margin-left: 8px !important;
  }
}

/* Large screens */
@media (min-width: 1600px) {
  .studies-cards-grid {
    grid-template-columns: repeat(auto-fill, minmax(380px, 1fr));
  }
}

/* Print */
@media print {

  .study-table-filters,
  .study-table-actions,
  .remote-browsing-warning {
    display: none !important;
  }

  .study-table {
    box-shadow: none;
  }

  .study-table td,
  .study-table th {
    border: 1px solid #000 !important;
  }
}
</style>