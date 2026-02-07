<script>
import StudyItem from "./StudyItem.vue";
import StudyCard from "./StudyCard.vue";
import ResourceButtonGroup from "./ResourceButtonGroup.vue";
import LabelsEditor from "./LabelsEditor.vue";
import Toasts from "./Toasts.vue";

import { mapState, mapGetters } from "vuex";
import { baseOe2Url } from "../globalConfigurations";
import { translateDicomTag } from "../locales/i18n";
import dateHelpers from "../helpers/date-helpers";
import $ from "jquery";
import {
  endOfMonth,
  endOfYear,
  startOfMonth,
  startOfYear,
  subMonths,
  subDays,
  startOfWeek,
  endOfWeek,
  subYears,
} from "date-fns";
import api from "../orthancApi";
import { ref } from "vue";
import SourceType from "../helpers/source-type";
import { ObserveVisibility as vObserveVisibility } from "vue3-observe-visibility";
import { nextTick } from "vue";

const Status = Object.freeze({
  UNDEFINED: 0,
  LOADING_MOST_RECENT_STUDIES: 1,
  DISPLAYING_MOST_RECENT_STUDIES: 2,
  DISPLAYING_FILTERED_STUDIES: 3,
});

document._allowedFilters = [
  "StudyDate",
  "StudyTime",
  "AccessionNumber",
  "PatientID",
  "PatientName",
  "PatientBirthDate",
  "StudyInstanceUID",
  "StudyID",
  "StudyDescription",
  "ModalitiesInStudy",
  "labels",
];

document._studyColumns = {
  StudyDate: {
    width: "7%",
    isOrderable: true,
  },
  AccessionNumber: {
    width: "10%",
    placeholder: "AE1234",
    isOrderable: true,
  },
  PatientID: {
    width: "11%",
    placeholder: "ID1234",
    isOrderable: true,
  },
  PatientName: {
    width: "20%",
    placeholder: "Dementev^Ivan",
    isOrderable: true,
  },
  PatientBirthDate: {
    width: "7%",
    isOrderable: true,
  },
  StudyDescription: {
    width: "15%",
    placeholder: "CHEST",
    isOrderable: true,
  },
  modalities: {
    width: "6%",
    isOrderable: false,
  },
  seriesCount: {
    width: "4%",
    isOrderable: false,
  },
  instancesCount: {
    width: "4%",
    isOrderable: false,
  },
  seriesAndInstancesCount: {
    width: "7%",
    isOrderable: false,
  },
  undefined: {
    width: "10%",
    isOrderable: false,
  },
  // columns that are not included by default but that are commonly added
  PatientSex: {
    width: "7%",
    placeholder: "",
    isOrderable: true,
  },
  OtherPatientIDs: {
    width: "10%",
    placeholder: "",
    isOrderable: true,
  },
  InstitutionName: {
    width: "12%",
    placeholder: "",
    isOrderable: true,
  },
  ReferringPhysician: {
    width: "10%",
    placeholder: "",
    isOrderable: true,
  },
  RequestingPhysician: {
    width: "10%",
    placeholder: "",
    isOrderable: true,
  },
  ManufacturerModelName: {
    width: "10%",
    placeholder: "",
    isOrderable: true,
  },
};

export default {
  props: [],
  emits: ["deletedStudy"],
  data() {
    return {
      viewMode: localStorage.getItem('studyViewMode') || 'table',
      activeModalityFilter: null,
      cardViewQuickFilter: 'all',
      filterStudyDate: "",
      cardViewSort: 'date-desc',
      isMobilePortrait: false,
      filterStudyDateForDatePicker: "",
      filterPatientBirthDate: "",
      filterPatientBirthDateForDatePicker: "",
      filterModalities: {},
      filterGenericTags: {},
      oldFilterGenericTags: {},
      filterLabels: [],
      currentOrderByTag: null,
      currentOrderDirection: "ASC",
      filterOrderBy: [
        { Type: "Metadata", Key: "LastUpdate", Direction: "DESC" },
      ],
      allModalities: true,
      noneModalities: false,
      updatingFilterUi: false,
      updatingRouteWithoutReload: false,
      initializingModalityFilter: false,
      searchTimerHandler: {},
      columns: document._studyColumns,
      datePickerPresetRanges: document._datePickerPresetRanges,
      allSelected: false,
      isPartialSelected: false,
      mostRecentStudiesIds: [],
      shouldStopLoadingMostRecentStudies: false,
      status: Status.UNDEFINED,
      sourceType: SourceType.LOCAL_ORTHANC,
      remoteSource: null,
      showMultiLabelsFilter: false,
      multiLabelsFilterLabelsConstraint: "All",
      showStudiesWithoutLabels: false,
      multiLabelsComponentKey: 0,
    };
  },
  computed: {
    // === НОВЫЕ COMPUTED ДЛЯ ОТОБРАЖЕНИЯ АКТИВНЫХ ФИЛЬТРОВ ===
    activeQuickFilterText() {
      if (!this.isCardView || this.cardViewQuickFilter === 'all') return null;

      const filterTexts = {
        'today': this.$t('date_picker.today'),
        'week': this.$t('date_picker.last_week'),
        'month': this.$t('date_picker.last_month'),
        'year': this.$t('date_picker.last_12_months') || this.$t('last_year')
      };

      return filterTexts[this.cardViewQuickFilter] || null;
    },

    hasActiveTextFilters() {
      if (!this.filterGenericTags) return false;
      for (const [tag, value] of Object.entries(this.filterGenericTags)) {
        if (value && value.trim && value.trim() !== '') {
          return true;
        }
      }
      return false;
    },

    activeTextFiltersList() {
      const filters = [];
      const labelMap = {
        'PatientName': this.$t('patient_name') || 'ФИО',
        'PatientID': this.$t('patient_id') || 'ID',
        'AccessionNumber': this.$t('accession_number') || 'Accession #',
        'StudyDescription': this.$t('study_description') || 'Описание',
        'StudyID': this.$t('study_id') || 'Study ID',
        'StudyInstanceUID': 'Study UID',
        'PatientSex': this.$t('patient_sex') || 'Пол',
        'OtherPatientIDs': this.$t('other_patient_ids') || 'Другие ID',
        'InstitutionName': this.$t('institution_name') || 'Учреждение',
        'ReferringPhysician': this.$t('referring_physician') || 'Направивший',
        'RequestingPhysician': this.$t('requesting_physician') || 'Запрашивающий',
        'ManufacturerModelName': this.$t('manufacturer_model') || 'Аппарат'
      };

      for (const [tag, value] of Object.entries(this.filterGenericTags)) {
        if (value && value.trim && value.trim() !== '') {
          filters.push({
            tag: tag,
            label: labelMap[tag] || tag,
            value: value
          });
        }
      }
      return filters;
    },

    hasActiveBirthDateFilter() {
      return !!this.filterPatientBirthDate && this.filterPatientBirthDate !== '';
    },

    formattedBirthDateFilter() {
      if (!this.filterPatientBirthDate) return '';
      if (this.filterPatientBirthDate.includes('-')) {
        const [start, end] = this.filterPatientBirthDate.split('-');
        return `${this.formatDicomDateShort(start)} - ${this.formatDicomDateShort(end)}`;
      }
      return this.formatDicomDateShort(this.filterPatientBirthDate);
    },

    hasAnyActiveFiltersExceptDate() {
      return this.hasActiveTextFilters ||
        this.hasActiveBirthDateFilter ||
        (this.filterLabels && this.filterLabels.length > 0) ||
        (!this.isCardView && this.activeModalityFilter);
    },

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
    ...mapGetters([
      "studies/isFilterEmpty",
      "studies/isMostRecentOrdering",
    ]),
    isCardView() {
      if (this.isMobilePortrait) return true;
      return this.viewMode === 'cards';
    },
    isTableView() {
      if (this.isMobilePortrait) return false;
      return this.viewMode === 'table';
    },
    cardViewSortOptions() {
      return [
        { value: 'date-desc', label: '↓📅', title: this.$t('sort_date_newest') },
        { value: 'date-asc', label: '↑📅', title: this.$t('sort_date_oldest') },
        { value: 'patient-asc', label: 'A→Z 👤', title: this.$t('sort_patient_az') },
        { value: 'patient-desc', label: 'Z→A 👤', title: this.$t('sort_patient_za') },
        { value: 'modality', label: '🩻', title: this.$t('sort_modality') },
        { value: 'images-desc', label: '🖼️↓', title: this.$t('sort_images_most') },
      ];
    },
    sortedStudiesForCards() {
      if (!this.isCardView) return this.studiesIds;
      const studies = this.$store.state.studies.studies;
      const studyMap = new Map();
      for (const study of studies) {
        studyMap.set(study.ID, study);
      }
      let sorted = [...this.studiesIds];
      const getStudy = (id) => studyMap.get(id);

      switch (this.cardViewSort) {
        case 'date-desc':
          sorted.sort((a, b) => {
            const dateA = getStudy(a)?.MainDicomTags?.StudyDate || '';
            const dateB = getStudy(b)?.MainDicomTags?.StudyDate || '';
            return dateB.localeCompare(dateA);
          });
          break;
        case 'date-asc':
          sorted.sort((a, b) => {
            const dateA = getStudy(a)?.MainDicomTags?.StudyDate || '';
            const dateB = getStudy(b)?.MainDicomTags?.StudyDate || '';
            return dateA.localeCompare(dateB);
          });
          break;
        case 'patient-asc':
          sorted.sort((a, b) => {
            const nameA = getStudy(a)?.PatientMainDicomTags?.PatientName || '';
            const nameB = getStudy(b)?.PatientMainDicomTags?.PatientName || '';
            return nameA.localeCompare(nameB);
          });
          break;
        case 'patient-desc':
          sorted.sort((a, b) => {
            const nameA = getStudy(a)?.PatientMainDicomTags?.PatientName || '';
            const nameB = getStudy(b)?.PatientMainDicomTags?.PatientName || '';
            return nameB.localeCompare(nameA);
          });
          break;
        case 'modality':
          sorted.sort((a, b) => {
            const modA = getStudy(a)?.RequestedTags?.ModalitiesInStudy || '';
            const modB = getStudy(b)?.RequestedTags?.ModalitiesInStudy || '';
            return modA.localeCompare(modB);
          });
          break;
        case 'images-desc':
          sorted.sort((a, b) => {
            const countA = parseInt(getStudy(a)?.RequestedTags?.NumberOfStudyRelatedInstances || '0');
            const countB = parseInt(getStudy(b)?.RequestedTags?.NumberOfStudyRelatedInstances || '0');
            return countB - countA;
          });
          break;
      }
      return sorted;
    },
    hasQuickActionsColumn() {
      return (
        this.sourceType == SourceType.LOCAL_ORTHANC &&
        (this.uiOptions.EnableDownloadQuickButton || this.uiOptions.EnableSendQuickButton)
      );
    },
    notShowingAllResults() {
      if (this.sourceType == SourceType.LOCAL_ORTHANC && !this.hasExtendedFind) {
        if (this.studiesIds.length >= this.statistics.CountStudies) {
          return false;
        }
        return this.studiesIds.length == this.uiOptions.MaxStudiesDisplayed;
      }
      return false;
    },
    isDisplayingMostRecentStudies() {
      return this.status == Status.DISPLAYING_MOST_RECENT_STUDIES;
    },
    isLoadingMostRecentStudies() {
      return this.status == Status.LOADING_MOST_RECENT_STUDIES;
    },
    isDarkMode() {
      let bootstrapTheme = document.documentElement.getAttribute("data-bs-theme");
      bootstrapTheme = getComputedStyle(document.documentElement).getPropertyValue("--bootstrap-theme");
      return bootstrapTheme == "dark";
    },
    isRemoteDicom() {
      return this.sourceType == SourceType.REMOTE_DICOM;
    },
    isRemoteDicomWeb() {
      return this.sourceType == SourceType.REMOTE_DICOM_WEB;
    },
    isMultiLabelsFilterVisible() {
      return (
        this.sourceType == SourceType.LOCAL_ORTHANC &&
        this.showMultiLabelsFilter &&
        this.uiOptions.EnableMultiLabelsSearch
      );
    },
    isSearchAsYouTypeEnabled() {
      if (this.sourceType == SourceType.LOCAL_ORTHANC) {
        return this.uiOptions.StudyListSearchMode == "search-as-you-type";
      }
      return false;
    },
    isSearchButtonEnabled() {
      if (this.sourceType == SourceType.LOCAL_ORTHANC) {
        return this.uiOptions.StudyListSearchMode == "search-button";
      }
      return true;
    },
    showEmptyStudyListIfNoSearch() {
      if (this.sourceType == SourceType.LOCAL_ORTHANC) {
        return this.uiOptions.StudyListContentIfNoSearch == "empty";
      }
      return true;
    },
    isStudyListEmpty() {
      return this.studiesIds.length == 0;
    },
    datePickerFormat() {
      return this.uiOptions.DateFormat;
    },
    hasPrimaryViewerIcon() {
      return (
        this.sourceType == SourceType.LOCAL_ORTHANC &&
        this.uiOptions.EnableViewerQuickButton
      );
    },
    hasPdfReportIcon() {
      return (
        this.sourceType == SourceType.LOCAL_ORTHANC &&
        this.uiOptions.EnableReportQuickButton
      );
    },
    selectedStudiesCount() {
      if (this.selectedStudiesIds.length > 0) {
        return this.selectedStudiesIds.length;
      }
      return "";
    },
    colSpanBeforeMultiLabelsFilter() {
      let span = 1;
      if (this.hasPrimaryViewerIcon) span++;
      if (this.hasPdfReportIcon) span++;
      return span;
    },
    colSpanMultiLabelsFilter() {
      if (this.uiOptions && this.uiOptions.StudyListColumns) {
        let totalColumnsCount = this.uiOptions.StudyListColumns.length + 1;
        if (this.hasPrimaryViewerIcon) totalColumnsCount++;
        if (this.hasPdfReportIcon) totalColumnsCount++;
        return (
          totalColumnsCount -
          this.colSpanBeforeMultiLabelsFilter -
          this.colSpanAfterMultiLabelsFilter
        );
      }
      return 4;
    },
    colSpanAfterMultiLabelsFilter() {
      return 3;
    },
    widthColum1() {
      if (this.colSpanClearFilter == 1) {
        return "4%";
      }
      return "3%";
    },
    colSpanClearFilter() {
      if (this.sourceType != SourceType.LOCAL_ORTHANC) {
        return 1;
      }
      let span = 1;
      if (this.hasPrimaryViewerIcon) span++;
      if (this.hasPdfReportIcon) span++;
      return span;
    },
  },
  watch: {
    $route: async function () {
      if (!this.updatingRouteWithoutReload) {
        this.updateFilterFromRoute(this.$route.query);
      }
    },
    isConfigurationLoaded(newValue, oldValue) {
      this.initModalityFilter();
      for (const tag of this.uiOptions.StudyListColumns) {
        if (
          [
            "StudyDate",
            "PatientBirthDate",
            "modalities",
            "seriesCount",
            "instancesCount",
            "seriesAndInstancesCount",
          ].indexOf(tag) == -1
        ) {
          this.filterGenericTags[tag] = "";
        }
      }
      this.updateFilterFromRoute(this.$route.query);
      setTimeout(() => {
        this.showMultiLabelsFilter = true;
      }, 300);
    },
    filterModalities: {
      handler(newValue, oldValue) {
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
      handler(newValue, oldValue) {
        // oldValue is the same as newValue for deep watchers
        for (const [k, v] of Object.entries(this.filterGenericTags)) {
          let oldValue = null;
          if (k in this.oldFilterGenericTags) {
            oldValue = this.oldFilterGenericTags[k];
          }
          this.updateFilter(k, v, oldValue);
          this.oldFilterGenericTags[k] = v;
        }
      },
      deep: true,
    },
    filterStudyDate(newValue, oldValue) {
      if (!this.updatingFilterUi) {
        this.updateFilter("StudyDate", newValue, oldValue);
      }
    },
    filterStudyDateForDatePicker(newValue, oldValue) {
      let dicomNewValue = dateHelpers.dicomDateFromDatePicker(newValue);
      if (dicomNewValue == null) {
        dicomNewValue = "";
      }
      this.filterStudyDate = dicomNewValue;
    },
    filterPatientBirthDate(newValue, oldValue) {
      this.updateFilter("PatientBirthDate", newValue, oldValue);
    },
    filterPatientBirthDateForDatePicker(newValue, oldValue) {
      let dicomNewValue = dateHelpers.dicomDateFromDatePicker(newValue);
      if (dicomNewValue == null) {
        dicomNewValue = "";
      }
      this.filterPatientBirthDate = dicomNewValue;
    },
    async multiLabelsFilterLabelsConstraint(newValue, oldValue) {
      if (!this.updatingFilterUi) {
        if (this.isSearchAsYouTypeEnabled) {
          if (this.multiLabelsFilterLabelsConstraint == "None") {  // ← ИСПРАВЛЕНО: добавлен this.
            await this.$store.dispatch("studies/updateLabelFilterNoReload", {
              labels: [],
              constraint: this.multiLabelsFilterLabelsConstraint,
            });
          } else {
            await this.$store.dispatch("studies/updateLabelFilterNoReload", {
              labels: this.filterLabels,
              constraint: this.multiLabelsFilterLabelsConstraint,
            });
          }
          this.updateUrlNoReload();
          this.reloadStudyList();
        }
      }
    },
    selectedStudiesIds: {
      handler(oldValue, newValue) {
        this.updateSelectAll();
      },
      deep: true,
    },
    allLabels(newValue, oldValue) {
      this.multiLabelsComponentKey++;
    },
  },
  async created() {
    this.messageBus.on("language-changed", this.translateDatePicker);
    window.addEventListener('mobile-portrait-mode', this.handleMobilePortraitMode);
    this.checkMobilePortrait();

    if (this.isConfigurationLoaded) {
      setTimeout(() => {
        this.showMultiLabelsFilter = true;
      }, 300);
    }
  },
  async mounted() {
    document.addEventListener('click', this.handleOutsideClick);
    window.addEventListener('resize', this.checkMobilePortrait);
  },
  beforeUnmount() {
    document.removeEventListener('click', this.handleOutsideClick);
    window.removeEventListener('mobile-portrait-mode', this.handleMobilePortraitMode);
    window.removeEventListener('resize', this.checkMobilePortrait);
  },
  methods: {
    // === НОВЫЕ МЕТОДЫ ДЛЯ ФИЛЬТРОВ ===
    formatDicomDateShort(dicomDate) {
      if (!dicomDate || dicomDate.length !== 8) return dicomDate;
      const year = dicomDate.substring(0, 4);
      const month = dicomDate.substring(4, 6);
      const day = dicomDate.substring(6, 8);
      return `${day}.${month}.${year}`;
    },
    clearTextFilter(tagName) {
      this.filterGenericTags[tagName] = '';
      this._updateFilter(tagName, '');
    },
    clearBirthDateFilter() {
      this.filterPatientBirthDate = '';
      this.filterPatientBirthDateForDatePicker = null;
      this._updateFilter('PatientBirthDate', '');
    },
    clearLabelsFilter() {
      this.filterLabels = [];
      this.$store.dispatch("studies/updateLabelFilterNoReload", {
        labels: [],
        constraint: "All"
      });
      this.updateUrlNoReload();
      this.reloadStudyList();
    },
    clearAllFiltersFromAlert() {
      this.clearFilters();
    },
    handleOutsideClick() {
      // Stub для совместимости
    },
    checkMobilePortrait() {
      const isMobile = window.matchMedia("(max-width: 768px)").matches;
      const isPortrait = window.matchMedia("(orientation: portrait)").matches;
      this.isMobilePortrait = isMobile && isPortrait;
      if (this.isMobilePortrait) {
        this.viewMode = 'cards';
      }
    },
    handleMobilePortraitMode(e) {
      this.isMobilePortrait = e.detail.isMobilePortrait;
      if (this.isMobilePortrait) {
        this.viewMode = 'cards';
      }
    },
    clearActiveModalityFilter() {
      this.activeModalityFilter = null;
      for (const modality of this.uiOptions.ModalitiesFilter) {
        this.filterModalities[modality] = true;
      }
      this.allModalities = true;
      this.noneModalities = false;
      this.$store.dispatch("studies/updateFilterNoReload", {
        dicomTagName: "ModalitiesInStudy",
        value: "",
      });
      this.updateUrlNoReload();
      this.reloadStudyList();
    },
    clearModalityFilter() {
      if (this.uiOptions.StudyListColumns) {
        for (const modality of this.uiOptions.ModalitiesFilter) {
          this.filterModalities[modality] = true;
        }
      }
    },
    onFilterModality(modality) {
      const modalitiesToSet = modality.split(/[\\,]+/).map(m => m.trim()).filter(m => m);
      if (modalitiesToSet.length === 0) return;
      this.activeModalityFilter = modalitiesToSet.join(", ");
      for (const key of Object.keys(this.filterModalities)) {
        this.filterModalities[key] = false;
      }
      for (const mod of modalitiesToSet) {
        if (mod in this.filterModalities) {
          this.filterModalities[mod] = true;
        }
      }
      this.allModalities = false;
      this.noneModalities = false;
      const modalityFilter = modalitiesToSet.join("\\");
      this.$store.dispatch("studies/updateFilterNoReload", {
        dicomTagName: "ModalitiesInStudy",
        value: modalityFilter,
      });
      this.updateUrlNoReload();
      this.reloadStudyList();
    },
    updateSelectAll() {
      if (this.selectedStudiesIds.length == 0) {
        this.allSelected = false;
        this.isPartialSelected = false;
      } else if (this.selectedStudiesIds.length == this.studiesIds.length) {
        this.allSelected = true;
        this.isPartialSelected = false;
      } else {
        this.allSelected = "";
        this.isPartialSelected = true;
      }
    },
    clickSelectAll() {
      if (this.allSelected == "" || !this.allSelected) {
        this.$store.dispatch("studies/selectAllStudies", { isSelected: true });
        this.messageBus.emit("selected-all");
      } else {
        this.$store.dispatch("studies/selectAllStudies", { isSelected: false });
        this.messageBus.emit("unselected-all");
      }
    },
    translateDatePicker(languageKey) {
      for (let i in document._datePickerPresetRanges) {
        document._datePickerPresetRanges[i].label = this.$t(
          document._datePickerPresetRanges[i].tLabel,
        );
      }
    },
    columnTitle(tagName) {
      if (tagName == "seriesCount") {
        return this.$i18n.t("series_count_header");
      } else if (tagName == "instancesCount") {
        return this.$i18n.t("instances_count_header");
      } else if (tagName == "seriesAndInstancesCount") {
        return this.$i18n.t("series_and_instances_count_header");
      } else if (tagName == "modalities") {
        return translateDicomTag(this.$i18n.t, this.$i18n.te, "ModalitiesInStudy");
      } else {
        return translateDicomTag(this.$i18n.t, this.$i18n.te, tagName);
      }
    },
    columnTooltip(tagName) {
      return this.columnTitle(tagName);
    },
    columnWidth(tagName) {
      if (tagName in this.columns) {
        return this.columns[tagName].width;
      }
      return this.columns["undefined"].width;
    },
    isOrderable(tagName) {
      if (this.sourceType != SourceType.LOCAL_ORTHANC || !this.hasExtendedFind) {
        return false;
      }
      if (tagName in document._studyColumns) {
        return document._studyColumns[tagName].isOrderable;
      }
      return false;
    },
    isOrderTagUp(tagName) {
      return (
        tagName == this.currentOrderByTag &&
        this.currentOrderDirection == "DESC"
      );
    },
    isOrderTagDown(tagName) {
      return (
        tagName == this.currentOrderByTag && this.currentOrderDirection == "ASC"
      );
    },
    toggleOrder(ev, tagName) {
      ev.preventDefault();
      ev.stopPropagation();
      if (this.currentOrderByTag == tagName) {
        this.currentOrderDirection = this.currentOrderDirection == "ASC" ? "DESC" : "ASC";
      } else {
        this.currentOrderByTag = tagName;
        this.currentOrderDirection = "ASC";
      }
      let o = {
        Type: "DicomTag",
        Key: this.currentOrderByTag,
        Direction: this.currentOrderDirection,
      };
      this.filterOrderBy = [o].concat(
        this.filterOrderBy.filter((i) => i["Type"] != "DicomTag"),
      );
      this._updateOrderBy(true);
    },
    initModalityFilter() {
      this.initializingModalityFilter = true;
      this.filterModalities = {};
      for (const modality of this.uiOptions.ModalitiesFilter) {
        this.filterModalities[modality] = true;
      }
      this.initializingModalityFilter = false;
    },
    getModalityFilter() {
      if (this.filterModalities === undefined) {
        return "";
      }
      let modalityFilter = "";
      let allSelected = true;
      let selected = [];
      for (const [key, value] of Object.entries(this.filterModalities)) {
        allSelected &= value;
        if (value) {
          selected.push(key);
        }
      }
      if (allSelected) {
        this.allModalities = true;
        this.noneModalities = false;
        return "";
      } else if (selected.length == 0) {
        this.allModalities = false;
        this.noneModalities = true;
        return "NONE";
      } else {
        this.allModalities = false;
        this.noneModalities = false;
        return selected.join("\\");
      }
    },
    updateFilter(dicomTagName, newValue, oldValue) {
      if (this.updatingFilterUi) {
        return;
      }
      if (!this.isSearchAsYouTypeEnabled) {
        return;
      }
      if (dicomTagName == "ModalitiesInStudy" && oldValue == null) {
        this._updateFilter(dicomTagName, newValue);
        return;
      }
      if (newValue.length >= this.uiOptions.StudyListSearchAsYouTypeMinChars) {
        if (this.searchTimerHandler[dicomTagName]) {
          clearTimeout(this.searchTimerHandler[dicomTagName]);
        }
        this.searchTimerHandler[dicomTagName] = setTimeout(() => {
          this._updateFilter(dicomTagName, newValue);
        }, this.uiOptions.StudyListSearchAsYouTypeDelay);
      } else if (
        oldValue &&
        newValue.length < oldValue.length &&
        oldValue.length >= this.uiOptions.StudyListSearchAsYouTypeMinChars
      ) {
        this.searchTimerHandler[dicomTagName] = setTimeout(() => {
          this._updateFilter(dicomTagName, "");
        }, this.uiOptions.StudyListSearchAsYouTypeDelay);
      }
    },
    clipFilter(dicomTagName, value) {
      if (this.isFilterLongEnough(dicomTagName, value)) {
        return value;
      }
      return "";
    },
    getMinimalFilterLength(dicomTagName) {
      if (
        ["AccessionNumber", "PatientName", "PatientID", "StudyDescription"].indexOf(dicomTagName) != -1
      ) {
        if (this.isSearchAsYouTypeEnabled) {
          return this.uiOptions.StudyListSearchAsYouTypeMinChars;
        }
      } else if (["PatientBirthDate", "StudyDate"].indexOf(dicomTagName) != -1) {
        return 8;
      }
      return 0;
    },
    isFilterLongEnough(dicomTagName, value) {
      return value.length >= this.getMinimalFilterLength(dicomTagName);
    },
    getFilterClass(dicomTagName) {
      const value = this.getFilterValue(dicomTagName);
      if (
        value != null &&
        value.length > 0 &&
        !this.isFilterLongEnough(dicomTagName, value)
      ) {
        return "is-invalid-filter";
      }
      return "";
    },
    hasFilter(tagName) {
      return (
        ["seriesCount", "instancesCount", "seriesAndInstancesCount"].indexOf(tagName) == -1
      );
    },
    getFilterPlaceholder(tagName) {
      if (tagName in this.columns && this.columns[tagName].placeholder) {
        return this.columns[tagName].placeholder;
      }
      return "search-text";
    },
    getFilterValue(dicomTagName) {
      if (!this.isConfigurationLoaded) {
        return null;
      }
      if (dicomTagName == "StudyDate") {
        return this.filterStudyDate;
      } else if (dicomTagName == "PatientBirthDate") {
        return this.filterPatientBirthDate;
      } else if (dicomTagName == "ModalitiesInStudy") {
        console.error("getFilterValue ModalitiesInStudy");
      } else {
        return this.filterGenericTags[dicomTagName];
      }
    },
    _updateFilter(dicomTagName, value) {
      this.searchTimerHandler[dicomTagName] = null;
      this.$store.dispatch("studies/updateFilterNoReload", {
        dicomTagName: dicomTagName,
        value: value,
      });
      this.updateUrlNoReload();
      this.reloadStudyList();
    },
    _updateOrderBy(reloadNow) {
      this.$store.dispatch("studies/updateOrderByNoReload", {
        orderBy: this.filterOrderBy,
      });
      if (reloadNow) {
        this.updateUrlNoReload();
        this.reloadStudyList();
      }
    },
    updateOrderBy(orderString, reloadNow) {
      console.log("updateOrderBy", orderString);
      this.filterOrderBy = [];
      this.currentOrderByTag = null;
      this.currentOrderDirection = "ASC";
      let orders = orderString.split(";");
      for (let order of orders) {
        let o = order.split(",");
        this.filterOrderBy.push({ Type: o[0], Key: o[1], Direction: o[2] });
        if (o[0] == "DicomTag" && this.currentOrderByTag == null) {
          this.currentOrderByTag = o[1];
          this.currentOrderDirection = o[2];
        }
      }
      this._updateOrderBy(reloadNow);
    },
    async updateFilterFromRoute(filters) {
      this.updatingFilterUi = true;
      await this.$store.dispatch("studies/clearStudies");
      await this.$store.dispatch("studies/clearFilterNoReload");
      var keyValueFilters = {};
      if ("source-type" in filters && "remote-source" in filters) {
        if (filters["source-type"].toLowerCase() === "dicom") {
          this.sourceType = SourceType.REMOTE_DICOM;
        } else if (filters["source-type"].toLowerCase() === "dicom-web") {
          this.sourceType = SourceType.REMOTE_DICOM_WEB;
        }
        this.remoteSource = filters["remote-source"];
      } else {
        this.sourceType = SourceType.LOCAL_ORTHANC;
        this.remoteSource = null;
      }
      await this.$store.dispatch("studies/updateSource", {
        "source-type": this.sourceType,
        "remote-source": this.remoteSource,
      });
      let routeHasOrderBy = false;
      let labelsConstraint = filters["labels-constraint"] || "All";
      let routeHasShowStudiesWithoutLabels =
        Object.keys(filters).filter((k) => k == "without-labels").length > 0;
      for (const [filterKey, filterValue] of Object.entries(filters)) {
        if (filterKey == "labels" && !routeHasShowStudiesWithoutLabels) {
          const labels = filterValue.split(",");
          keyValueFilters[filterKey] = labels;
          await this.$store.dispatch("studies/updateLabelFilterNoReload", {
            labels: labels,
            constraint: labelsConstraint,
          });
        } else if (filterKey == "labels-constraint" && !routeHasShowStudiesWithoutLabels) {
          this.multiLabelsFilterLabelsConstraint = filterValue;
        } else if (filterKey == "order-by") {
          if (this.sourceType == SourceType.LOCAL_ORTHANC) {
            this.updateOrderBy(filterValue, false);
            routeHasOrderBy = true;
          }
        } else if (filterKey[0] === filterKey[0].toUpperCase()) {
          keyValueFilters[filterKey] = filterValue;
          await this.$store.dispatch("studies/updateFilterNoReload", {
            dicomTagName: filterKey,
            value: filterValue,
          });
        }
      }
      if (routeHasShowStudiesWithoutLabels) {
        await this.$store.dispatch("studies/updateLabelFilterNoReload", {
          labels: [],
          constraint: "None",
        });
        this.showStudiesWithoutLabels = true;
      } else {
        this.showStudiesWithoutLabels = false;
      }
      if (!routeHasOrderBy && this.uiOptions.DefaultOrdering) {
        const defaultOrdering = this.uiOptions.DefaultOrdering;
        console.log("Applying default ordering: ", defaultOrdering);
        this.updateOrderBy(defaultOrdering, false);
      }
      await this.updateFilterForm(keyValueFilters, labelsConstraint, this.showStudiesWithoutLabels);
      if (this.sourceType == SourceType.LOCAL_ORTHANC || !this["studies/isFilterEmpty"]) {
        await this.reloadStudyList();
      }
      this.multiLabelsComponentKey++;
      await nextTick();
      this.updatingFilterUi = false;
    },
    updateFilterForm(filters, labelsConstraint, showStudiesWithoutLabels) {
      this.emptyFilterForm();
      this.multiLabelsFilterLabelsConstraint = labelsConstraint;
      this.showStudiesWithoutLabels = showStudiesWithoutLabels;
      for (const [key, value] of Object.entries(filters)) {
        if (key == "labels") {
          this.filterLabels = value;
        } else if (key == "order-by") {
          let orders = value.split(";");
          for (let order of orders) {
            let s = order.split(",");
          }
        } else if (key == "StudyDate") {
          this.filterStudyDate = value;
          this.filterStudyDateForDatePicker = dateHelpers.parseDateForDatePicker(value);
        } else if (key == "PatientBirthDate") {
          this.filterPatientBirthDate = value;
          this.filterPatientBirthDateForDatePicker = dateHelpers.parseDateForDatePicker(value);
        } else if (key == "ModalitiesInStudy") {
          const modalities = value.split("\\");
          if (modalities.length > 0) {
            let allModalitiesInFilter = true;
            let noneModalitiesInFilter = true;
            for (const modality of this.uiOptions.ModalitiesFilter) {
              const isInFilter = modalities.indexOf(modality) != -1;
              this.filterModalities[modality] = isInFilter;
              allModalitiesInFilter &= isInFilter;
              noneModalitiesInFilter &= !isInFilter;
            }
            this.allModalities = allModalitiesInFilter;
            this.noneModalities = noneModalitiesInFilter;
          }
        } else {
          this.filterGenericTags[key] = value;
        }
      }
    },
    emptyFilterForm() {
      this.filterStudyDate = "";
      this.filterStudyDateForDatePicker = null;
      this.filterPatientBirthDate = "";
      this.multiLabelsFilterLabelsConstraint = "All";
      this.showStudiesWithoutLabels = false;
      this.cardViewQuickFilter = 'all';
      this.filterPatientBirthDateForDatePicker = null;
      this.filterGenericTags = {};
      if (this.uiOptions.StudyListColumns) {
        for (const tag of this.uiOptions.StudyListColumns) {
          if (
            ["StudyDate", "PatientBirthDate", "modalities", "seriesCount", "instancesCount", "seriesAndInstancesCount"].indexOf(tag) == -1
          ) {
            this.filterGenericTags[tag] = "";
          }
        }
      }
      this.filterLabels = [];
      this.clearModalityFilter();
    },
    isFilteringOnlyOnLabels() {
      let hasGenericTagFilter = false;
      for (const tag of this.uiOptions.StudyListColumns) {
        if (
          ["StudyDate", "PatientBirthDate", "modalities", "seriesCount", "instancesCount", "seriesAndInstancesCount"].indexOf(tag) == -1
        ) {
          if (this.filterGenericTags[tag] && this.filterGenericTags[tag] != "") {
            hasGenericTagFilter = true;
          }
        }
      }
      return (
        this.filterStudyDate == "" &&
        this.filterPatientBirthDate == "" &&
        !hasGenericTagFilter &&
        this.filterLabels.length > 0 &&
        this.filterOrderBy.length == 0
      );
    },
    async search() {
      if (this.isSearching) {
        await this.$store.dispatch("studies/cancelSearch");
      } else {
        await this.$store.dispatch("studies/clearFilterNoReload");
        for (const tag of this.uiOptions.StudyListColumns) {
          if (["modalities", "seriesCount", "instancesCount", "seriesAndInstancesCount"].indexOf(tag) == -1) {
            await this.$store.dispatch("studies/updateFilterNoReload", {
              dicomTagName: tag,
              value: this.getFilterValue(tag),
            });
          }
        }
        await this.$store.dispatch("studies/updateFilterNoReload", {
          dicomTagName: "ModalitiesInStudy",
          value: this.getModalityFilter(),
        });
        if (this.showStudiesWithoutLabels) {
          await this.$store.dispatch("studies/updateLabelFilterNoReload", {
            labels: [],
            constraint: "None",
          });
        } else {
          await this.$store.dispatch("studies/updateLabelFilterNoReload", {
            labels: this.filterLabels,
            constraint: this.multiLabelsFilterLabelsConstraint,
          });
        }
        await this.updateUrlNoReload();
        await this.reloadStudyList();
      }
    },
    async clearFilters() {
      await this.clearFiltersUi();
      await this.$store.dispatch("studies/clearFilterNoReload");
      this.activeModalityFilter = null;
      this.cardViewQuickFilter = 'all';
      if (this.sourceType == SourceType.LOCAL_ORTHANC) {
        this.reloadStudyList();
      }
    },
    async clearFiltersUi() {
      this.updatingFilterUi = true;
      this.emptyFilterForm();
      this.activeModalityFilter = null;
      this.cardViewQuickFilter = 'all';
      this.filterStudyDateForDatePicker = null;
      this.updateUrl();
      await nextTick();
      this.updatingFilterUi = false;
    },
    async toggleModalityFilter(ev) {
      const modality = ev.srcElement.getAttribute("data-value");
      let newValue = true;
      if (modality == "all") {
        newValue = true;
      } else if (modality == "none") {
        newValue = false;
      }
      for (const [key, value] of Object.entries(this.filterModalities)) {
        this.filterModalities[key] = newValue;
      }
      this.getModalityFilter();
    },
    modalityFilterClicked(ev) {
      ev.stopPropagation();
    },
    closeModalityFilter(ev) {
      $("#dropdown-modalities-button").click();
      ev.preventDefault();
      ev.stopPropagation();
    },
    async updateUrlNoReload() {
      this.updatingRouteWithoutReload = true;
      await this.updateUrl();
      this.updatingRouteWithoutReload = false;
    },
    async updateUrl() {
      let query = {};
      if (this.sourceType != SourceType.LOCAL_ORTHANC) {
        if (this.sourceType == SourceType.REMOTE_DICOM) {
          query["source-type"] = "dicom";
        } else if (this.sourceType == SourceType.REMOTE_DICOM_WEB) {
          query["source-type"] = "dicom-web";
        }
        query["remote-source"] = this.remoteSource;
      }
      if (this.clipFilter("StudyDate", this.filterStudyDate)) {
        query["StudyDate"] = this.filterStudyDate;
      }
      if (this.clipFilter("PatientBirthDate", this.filterPatientBirthDate)) {
        query["PatientBirthDate"] = this.filterPatientBirthDate;
      }
      if (this.getModalityFilter()) {
        query["ModalitiesInStudy"] = this.getModalityFilter();
      }
      for (const [k, v] of Object.entries(this.filterGenericTags)) {
        if (this.clipFilter(k, v)) {
          query[k] = v;
        }
      }
      if (this.filterLabels.length > 0) {
        query["labels"] = this.filterLabels.join(",");
        if (this.multiLabelsFilterLabelsConstraint != "All") {
          query["labels-constraint"] = this.multiLabelsFilterLabelsConstraint;
        }
      }
      if (this.filterOrderBy.length > 0 && this.sourceType == SourceType.LOCAL_ORTHANC) {
        let orders = [];
        for (let order of this.filterOrderBy) {
          orders.push([order["Type"], order["Key"], order["Direction"]].join(","));
        }
        query["order-by"] = orders.join(";");
      }
      let newUrl = "/filtered-studies?" + new URLSearchParams(query).toString();
      await this.$router.replace(newUrl);
    },
    async extendStudyList() {
      if (this.sourceType == SourceType.LOCAL_ORTHANC && this.hasExtendedFind) {
        await this.$store.dispatch("studies/extendFilteredStudies");
      }
    },
    async reloadStudyList() {
      if (this.sourceType == SourceType.LOCAL_ORTHANC && this.hasExtendedFind) {
        if (this.uiOptions.StudyListContentIfNoSearch == "empty") {
          this.status = Status.UNDEFINED;
          if (this["studies/isFilterEmpty"]) {
            await this.$store.dispatch("studies/clearStudies");
          } else {
            await this.$store.dispatch("studies/clearStudies");
            await this.$store.dispatch("studies/reloadFilteredStudies");
          }
        } else {
          if (this["studies/isMostRecentOrdering"]) {
            this.status = Status.LOADING_MOST_RECENT_STUDIES;
          }
          await this.$store.dispatch("studies/clearStudies");
          await this.$store.dispatch("studies/reloadFilteredStudies");
          if (this["studies/isMostRecentOrdering"]) {
            this.status = Status.DISPLAYING_MOST_RECENT_STUDIES;
          } else {
            this.status = Status.DISPLAYING_FILTERED_STUDIES;
          }
        }
      } else {
        const shouldShowMostRecentsWithLabel =
          this.uiOptions.StudyListContentIfNoSearch == "most-recents" &&
          this.isFilteringOnlyOnLabels();
        if (this.sourceType == SourceType.LOCAL_ORTHANC && (this["studies/isFilterEmpty"] || shouldShowMostRecentsWithLabel)) {
          await this.$store.dispatch("studies/clearStudies");
          if (this.uiOptions.StudyListContentIfNoSearch == "empty") {
            return;
          } else if (this.uiOptions.StudyListContentIfNoSearch == "most-recents" && this.hasExtendedFind) {
            const studies = await api.getMostRecentStudiesExtended(
              this.filterLabels.length > 0 ? this.filterLabels[0] : null,
            );
            for (const study of studies) {
              this.$store.dispatch("studies/addStudy", {
                studyId: study["ID"],
                study: study,
                reloadStats: false,
              });
            }
          } else if (this.uiOptions.StudyListContentIfNoSearch == "most-recents") {
            if (this.status == Status.LOADING_MOST_RECENT_STUDIES) {
              this.shouldStopLoadingMostRecentStudies = true;
              this.status = Status.DISPLAYING_MOST_RECENT_STUDIES;
            }
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
      let changes;
      let changesResponse;
      if (this.hasExtendedChanges) {
        changesResponse = await api.getChangesExtended(toChangeId, limit, ["NewStudy", "StableStudy"]);
        changes = changesResponse["Changes"];
      } else {
        changesResponse = await api.getChanges(toChangeId - limit, limit);
        changes = changesResponse["Changes"].reverse();
      }
      for (let change of changes) {
        if (
          (change["ChangeType"] == "NewStudy" || change["ChangeType"] == "StableStudy") &&
          !this.mostRecentStudiesIds.has(change["ID"])
        ) {
          if (this.shouldStopLoadingMostRecentStudies) {
            return;
          }
          try {
            const study = await api.getStudy(change["ID"]);
            if (this.filterLabels.length == 0 || this.filterLabels.filter((l) => study["Labels"].includes(l)).length > 0) {
              this.$store.dispatch("studies/addStudy", {
                studyId: change["ID"],
                study: study,
                reloadStats: false,
              });
            }
            this.mostRecentStudiesIds.add(change["ID"]);
            if (this.mostRecentStudiesIds.size == this.uiOptions.MaxStudiesDisplayed) {
              this.status = Status.DISPLAYING_MOST_RECENT_STUDIES;
              return;
            }
          } catch (err) {
            console.warn("Unable to load study - not authorized ?");
          }
        }
      }
      if (!this.shouldStopLoadingMostRecentStudies) {
        if (this.mostRecentStudiesIds.size < this.statistics.CountStudies) {
          if (this.hasExtendedChanges) {
            if (!changesResponse["Done"]) {
              setTimeout(() => {
                this.loadStudiesFromChange(changesResponse["First"], 1000);
              }, 1);
            }
          } else {
            if (toChangeId != changesResponse["First"]) {
              setTimeout(() => {
                this.loadStudiesFromChange(Math.max(0, toChangeId - 1000), 1000);
              }, 1);
            }
          }
        } else {
          this.status = Status.DISPLAYING_MOST_RECENT_STUDIES;
        }
      } else {
        this.status = Status.DISPLAYING_MOST_RECENT_STUDIES;
      }
    },
    onDeletedStudy(studyId) {
      this.$store.dispatch("studies/deleteStudy", { studyId: studyId });
    },
    visibilityChanged(isVisible, entry) {
      if (isVisible) {
        let studyId = entry.target.id;
        const lastStudyId = this.isCardView
          ? this.sortedStudiesForCards[this.sortedStudiesForCards.length - 1]
          : this.studiesIds[this.studiesIds.length - 1];
        if (studyId == lastStudyId) {
          console.log("Last element shown -> should load more studies");
          this.extendStudyList();
        }
      }
    },
    onMultiLabelsFilterChanged(newValues) {
      if (!this.updatingFilterUi) {
        this.filterLabels = newValues;
        if (this.isSearchAsYouTypeEnabled) {
          this.updateUrlNoReload();
          this.reloadStudyList();
        }
      }
    },
    toggleViewMode() {
      this.viewMode = this.viewMode === 'table' ? 'cards' : 'table';
      localStorage.setItem('studyViewMode', this.viewMode);
    },
    setViewMode(mode) {
      this.viewMode = mode;
      localStorage.setItem('studyViewMode', mode);
    },
    setCardSort(sortValue) {
      this.cardViewSort = sortValue;
    },
    setCardQuickFilter(filter) {
      this.updatingFilterUi = true;
      this.cardViewQuickFilter = filter;
      const formatToDicomDate = (date) => {
        const year = date.getFullYear();
        const month = String(date.getMonth() + 1).padStart(2, '0');
        const day = String(date.getDate()).padStart(2, '0');
        return `${year}${month}${day}`;
      };
      const today = new Date();
      today.setHours(0, 0, 0, 0);
      let dateValue = '';
      switch (filter) {
        case 'today':
          dateValue = formatToDicomDate(today);
          break;
        case 'week':
          const weekAgo = new Date(today);
          weekAgo.setDate(today.getDate() - 7);
          dateValue = `${formatToDicomDate(weekAgo)}-${formatToDicomDate(today)}`;
          break;
        case 'month':
          const monthAgo = new Date(today);
          monthAgo.setMonth(today.getMonth() - 1);
          dateValue = `${formatToDicomDate(monthAgo)}-${formatToDicomDate(today)}`;
          break;
        case 'year':
          const yearAgo = new Date(today);
          yearAgo.setFullYear(today.getFullYear() - 1);
          dateValue = `${formatToDicomDate(yearAgo)}-${formatToDicomDate(today)}`;
          break;
        case 'all':
        default:
          dateValue = '';
          break;
      }
      this.filterStudyDate = dateValue;
      if (dateValue && dateValue.includes('-')) {
        const [start, end] = dateValue.split('-');
        this.filterStudyDateForDatePicker = [
          new Date(start.slice(0, 4), start.slice(4, 6) - 1, start.slice(6, 8)),
          new Date(end.slice(0, 4), end.slice(4, 6) - 1, end.slice(6, 8))
        ];
      } else if (dateValue) {
        const date = new Date(dateValue.slice(0, 4), dateValue.slice(4, 6) - 1, dateValue.slice(6, 8));
        this.filterStudyDateForDatePicker = [date, date];
      } else {
        this.filterStudyDateForDatePicker = null;
      }
      this.updatingFilterUi = false;
      this._updateFilter('StudyDate', dateValue);
    },
  },
  components: { StudyItem, StudyCard, ResourceButtonGroup, LabelsEditor, Toasts },
};
</script>

<template>
  <div>
    <div v-if="isRemoteDicom || isRemoteDicomWeb" class="remote-browsing-warning">
      <div>
        <p v-if="isRemoteDicom" v-html="$t('remote_dicom_browsing', { source: remoteSource })"></p>
        <p v-if="isRemoteDicomWeb" v-html="$t('remote_dicom_web_browsing', { source: remoteSource })"></p>
      </div>
    </div>
    <table class="table table-sm study-table table-borderless">
      <thead class="sticky-top">
        <tr class="study-column-titles">
          <th :width="widthColum1" max-width="40px" scope="col"></th>
          <th v-if="hasPrimaryViewerIcon" scope="col" class="download-buttons-group"></th>
          <th v-if="hasQuickActionsColumn" width="4%" max-width="70px" scope="col"></th>
          <th v-if="hasPdfReportIcon" width="3%" max-width="30px" scope="col"></th>
          <th v-for="columnTag in uiOptions.StudyListColumns" :key="columnTag" data-bs-toggle="tooltip"
            v-bind:title="columnTooltip(columnTag)" v-bind:width="columnWidth(columnTag)" class="study-table-title">
            <div class="title-container">
              <div v-if="isOrderable(columnTag)" class="title-text is-orderable"
                @click="toggleOrder($event, columnTag)">
                {{ columnTitle(columnTag) }}
              </div>
              <div v-if="!isOrderable(columnTag)" class="title-text">
                {{ columnTitle(columnTag) }}
              </div>
              <div v-if="isOrderTagUp(columnTag)" class="title-arrow">
                <i class="bi bi-arrow-up"></i>
              </div>
              <div v-if="isOrderTagDown(columnTag)" class="title-arrow">
                <i class="bi bi-arrow-down"></i>
              </div>
            </div>
          </th>
        </tr>
        <tr class="study-table-filters" v-on:keyup.enter="search">
          <th scope="col" :colspan="colSpanClearFilter">
            <div class="d-flex justify-content-end"><button @click="clearFilters" type="button"
                class="form-control study-list-filter btn filter-button btn-sm" data-bs-toggle="tooltip"
                :title="$t('clear_filter')">
                <i class="bi bi-x-lg"></i>
              </button></div>
          </th>
          <th v-for="columnTag in uiOptions.StudyListColumns" :key="columnTag">
            <div v-if="columnTag == 'StudyDate'">
              <Datepicker v-if="columnTag == 'StudyDate'" v-model="filterStudyDateForDatePicker"
                :enable-time-picker="false" range :preset-dates="datePickerPresetRanges" :format="datePickerFormat"
                :preview-format="datePickerFormat" text-input arrow-navigation hide-input-icon
                :highlight="{ weekdays: [6, 0] }" :dark="isDarkMode">
                <template #yearly="{ label, range, presetDate }">
                  <span @click="presetDate(range)">{{ label }}</span>
                </template>
              </Datepicker>
            </div>
            <div v-else-if="columnTag == 'modalities'" class="dropdown">
              <button type="button" class="btn btn-default btn-sm filter-button dropdown-toggle"
                data-bs-toggle="dropdown" id="dropdown-modalities-button" aria-expanded="false">
                <span class="fa fa-list"></span>&nbsp;<span class="caret"></span>
              </button>
              <ul class="dropdown-menu" aria-labelledby="dropdown-modalities-button" @click="modalityFilterClicked"
                id="modality-filter-dropdown">
                <li>
                  <label class="dropdown-item"><input type="checkbox" data-value="all" @click="toggleModalityFilter"
                      v-model="allModalities" />&nbsp;{{ $t("all_modalities") }}</label>
                </li>
                <li>
                  <label class="dropdown-item"><input type="checkbox" data-value="none" @click="toggleModalityFilter"
                      v-model="noneModalities" />&nbsp;{{ $t("no_modalities") }}</label>
                </li>
                <li>
                  <hr class="dropdown-divider" />
                </li>
                <li v-for="modality in uiOptions.ModalitiesFilter" :key="modality">
                  <label class="dropdown-item"><input type="checkbox" v-bind:data-value="modality"
                      v-model="filterModalities[modality]" />&nbsp;{{ modality }}</label>
                </li>
                <li>
                  <button class="btn btn-primary mx-5" @click="closeModalityFilter">
                    {{ $t("close") }}
                  </button>
                </li>
              </ul>
            </div>
            <div v-else-if="columnTag == 'PatientBirthDate'">
              <Datepicker v-model="filterPatientBirthDateForDatePicker" :enable-time-picker="false" range
                :format="datePickerFormat" hide-input-icon :preview-format="datePickerFormat" text-input
                arrow-navigation :highlight="{ weekdays: [6, 0] }" :dark="isDarkMode">
              </Datepicker>
            </div>
            <input v-else-if="hasFilter(columnTag)" type="text" class="form-control study-list-filter"
              v-model="this.filterGenericTags[columnTag]" v-bind:placeholder="getFilterPlaceholder(columnTag)"
              v-bind:class="getFilterClass(columnTag)" />
          </th>
        </tr>

        <tr v-if="isMultiLabelsFilterVisible" class="study-table-actions">
          <th :colspan="colSpanBeforeMultiLabelsFilter" scope="col">
            <div class="w-100 d-flex justify-content-end">
              <label class="form-check-label text-end" for="multiLabelsFilter">{{ $t("labels.study_details_title") }}
              </label>
            </div>
          </th>
          <th :colspan="colSpanMultiLabelsFilter" scope="col">
            <LabelsEditor id="multiLabelsFilter" :labels="filterLabels" :key="multiLabelsComponentKey" :studyId="null"
              @labelsUpdated="onMultiLabelsFilterChanged" :showTitle="false" :isFilter="true"></LabelsEditor>
          </th>
          <th :colspan="colSpanAfterMultiLabelsFilter" scope="col">
            <div class="w-100 d-flex">
              <input class="form-check-input ms-2 me-1" type="radio" name="multiLabelsFilterAll"
                id="multiLabelsFilterAll" value="All" v-model="multiLabelsFilterLabelsConstraint" />
              <label class="form-check-label" for="multiLabelsFilterAll">{{ $t("labels.filter_labels_constraint_all") }}
              </label>
              <input class="form-check-input ms-2 me-1" type="radio" name="multiLabelsFilterAny"
                id="multiLabelsFilterAny" value="Any" v-model="multiLabelsFilterLabelsConstraint" />
              <label class="form-check-label" for="multiLabelsFilterAny">{{ $t("labels.filter_labels_constraint_any") }}
              </label>
              <input v-if="canShowStudiesWithoutLabels" class="form-check-input ms-2 me-1" type="radio"
                name="multiLabelsFilterWithoutLabels" id="multiLabelsFilterWithoutLabels" value="None"
                v-model="multiLabelsFilterLabelsConstraint" />
              <label v-if="canShowStudiesWithoutLabels" class="form-check-label" for="multiLabelsFilterWithoutLabels">{{
                $t("labels.studies_without_labels") }}
              </label>
            </div>
          </th>
        </tr>



      </thead>
      <tr class="study-table-actions">
        <!-- Фиксированный первый блок: checkbox -->
        <th class="actions-fixed-left" width="5%" colspan="1" scope="col">
          <div class="form-check">
            <input class="form-check-input" type="checkbox" v-model="allSelected" :indeterminate="isPartialSelected"
              @click="clickSelectAll" />
            <span class="selected-count">{{ selectedStudiesCount }}</span>
          </div>
        </th>

        <!-- Адаптивная середина: все контролы -->
        <th class="actions-adaptive-center" width="90%"
          :colspan="colSpanMultiLabelsFilter + colSpanAfterMultiLabelsFilter" scope="col">
          <div class="adaptive-controls-wrapper">
            <!-- Левая группа: фильтры и сортировка -->
            <div class="center-left-group">
              <!-- Активный фильтр модальности (всегда виден, если есть) -->
              <div v-if="activeModalityFilter" class="active-modality-filter">
                <span class="badge bg-primary">
                  {{ activeModalityFilter }}
                  <i class="bi bi-x-circle" @click="clearActiveModalityFilter" title="Clear filter"></i>
                </span>
              </div>

              <!-- Quick filters (только для card view) -->
              <div v-if="isCardView" class="quick-filters">
                <button :title="$t('all')" class="quick-filter-btn" :class="{ active: cardViewQuickFilter === 'all' }"
                  @click="setCardQuickFilter('all')">
                  <i class="bi bi-list-ul"></i>
                </button>
                <button :title="$t('date_picker.today')" class="quick-filter-btn"
                  :class="{ active: cardViewQuickFilter === 'today' }" @click="setCardQuickFilter('today')">
                  <i class="bi bi-calendar-day"></i>
                </button>
                <button :title="$t('date_picker.last_week')" class="quick-filter-btn"
                  :class="{ active: cardViewQuickFilter === 'week' }" @click="setCardQuickFilter('week')">
                  <i class="bi bi-calendar-week"></i>
                </button>
                <button :title="$t('date_picker.last_month')" class="quick-filter-btn"
                  :class="{ active: cardViewQuickFilter === 'month' }" @click="setCardQuickFilter('month')">
                  <i class="bi bi-calendar-month"></i>
                </button>
                <button :title="$t('date_picker.last_12_months') || $t('last_year')" class="quick-filter-btn"
                  :class="{ active: cardViewQuickFilter === 'year' }" @click="setCardQuickFilter('year')">
                  <i class="bi bi-calendar"></i>
                </button>
              </div>

              <!-- Sort dropdown (только для card view) -->
              <div v-if="isCardView" class="card-sort-dropdown">
                <select id="cardSort" v-model="cardViewSort" class="form-select form-select-sm">
                  <option v-for="option in cardViewSortOptions" :key="option.value" :title="option.title"
                    :value="option.value">
                    {{ option.label }}
                  </option>
                </select>
              </div>
            </div>

            <!-- Блок кнопок действий (всегда виден) -->
            <div class="action-buttons-group">
              <ResourceButtonGroup :resourceLevel="'bulk'" smallIcons="true" />
            </div>

            <!-- Центральная часть: алерты -->
            <div class="alerts-container">
              <!-- Активные текстовые фильтры (ФИО, ID и т.д.) - показываем inline -->
              <div v-if="hasActiveTextFilters && !isSearching" class="active-filters-inline">
                <span v-for="filter in activeTextFiltersList" :key="filter.tag" class="filter-tag">
                  <span class="filter-tag-label">{{ filter.label }}:</span>
                  <span class="filter-tag-value" :title="filter.value">{{ filter.value }}</span>
                  <i class="bi bi-x-circle filter-tag-close" @click="clearTextFilter(filter.tag)"
                    :title="$t('clear_filter')"></i>
                </span>

                <!-- Фильтр даты рождения (если есть) -->
                <span v-if="hasActiveBirthDateFilter" class="filter-tag filter-tag-birthdate">
                  <span class="filter-tag-label">{{ $t('patient_birth_date') || 'Дата рождения' }}:</span>
                  <span class="filter-tag-value">{{ formattedBirthDateFilter }}</span>
                  <i class="bi bi-x-circle filter-tag-close" @click="clearBirthDateFilter()"
                    :title="$t('clear_filter')"></i>
                </span>

                <!-- Фильтр меток (если есть) -->
                <span v-if="filterLabels && filterLabels.length > 0" class="filter-tag filter-tag-labels">
                  <span class="filter-tag-label">{{ $t('labels.study_details_title') || 'Метки' }}:</span>
                  <span class="filter-tag-value">{{ filterLabels.join(', ') }}</span>
                  <i class="bi bi-x-circle filter-tag-close" @click="clearLabelsFilter()"
                    :title="$t('clear_filter')"></i>
                </span>

                <!-- Кнопка очистки всех фильтров -->
                <button v-if="hasAnyActiveFiltersExceptDate && !(isCardView && activeQuickFilterText)"
                  class="btn btn-sm btn-clear-all" @click="clearAllFiltersFromAlert()"
                  :title="$t('clear_all') || 'Очистить все'">
                  <i class="bi bi-trash"></i>
                </button>
              </div>
              <div v-if="!isSearching && isLoadingMostRecentStudies" class="alert alert-secondary study-list-alert"
                role="alert">
                <span class="spinner-border spinner-border-sm alert-icon" role="status" aria-hidden="true"></span>
                {{ $t("loading_most_recent_studies") }}
              </div>
              <div v-else-if="!isSearching && isDisplayingMostRecentStudies"
                class="alert alert-secondary study-list-alert" role="alert">
                <i class="bi bi-exclamation-triangle-fill alert-icon"></i>
                {{ $t("displaying_most_recent_studies") }}
              </div>
              <div v-else-if="!isSearching && notShowingAllResults" class="alert alert-danger study-list-alert"
                role="alert">
                <i class="bi bi-exclamation-triangle-fill alert-icon"></i>
                {{ $t("not_showing_all_results") }}
              </div>
              <div v-else-if="!isSearching && showEmptyStudyListIfNoSearch && this['studies/isFilterEmpty']"
                class="alert alert-warning study-list-alert" role="alert">
                <i class="bi bi-exclamation-triangle-fill alert-icon"></i>
                {{ $t("enter_search") }}
              </div>
              <div v-else-if="!isSearching && isStudyListEmpty" class="alert alert-warning study-list-alert"
                role="alert">
                <i class="bi bi-exclamation-triangle-fill alert-icon"></i>
                {{ $t("no_result_found") }}
              </div>
              <div v-else-if="isSearching" class="alert alert-secondary study-list-alert" role="alert">
                <span class="spinner-border spinner-border-sm alert-icon" role="status" aria-hidden="true"></span>
                {{ $t("searching") }}
              </div>
            </div>

            <!-- Кнопка поиска -->
            <div v-if="isSearchButtonEnabled" class="search-button-wrapper">
              <button @click="search" type="submit" class="btn btn-secondary search-button"
                :class="{ 'is-searching': isSearching, 'is-not-searching': !isSearching }" :title="$t('search')">
                <i v-if="!isSearching" class="fa-solid fa-magnifying-glass"></i>
                <span v-else class="spinner-border spinner-border-sm" role="status" aria-hidden="true"></span>
              </button>
            </div>
          </div>
        </th>

        <!-- Фиксированный правый блок: view switcher -->
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
        } : false" @deletedStudy="onDeletedStudy" @filter-modality="onFilterModality"></StudyItem>
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

    <Toasts />
  </div>
</template>

<style>
/* =============================================================================
   STUDYLIST.VUE - MODERN STYLES (D-MIS Design System)
   Inspired by SideBar.vue but adapted for light theme
   ============================================================================= */

/* CSS Variables */
:root {
  --filter-margin: 6px;
  --filter-padding: 8px;
  --study-border-radius: 8px;
  --study-transition: all 0.2s ease;
  --study-hover-shadow: 0 2px 8px rgba(59, 130, 246, 0.15);
  --study-active-shadow: 0 4px 12px rgba(59, 130, 246, 0.25);
}

/* =============================================================================
   REMOTE BROWSING WARNING
   ============================================================================= */
.remote-browsing-warning {
  background: linear-gradient(135deg, #3b82f6 0%, #8b5cf6 100%);
  text-align: center;
  font-weight: 600;
  height: 2.5rem;
  line-height: 2.5rem;
  color: white;
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
  font-family:
    "Segoe UI",
    system-ui,
    -apple-system,
    sans-serif;
  letter-spacing: 0.5px;
}

.remote-browsing-warning p {
  margin: 0;
  padding: 0;
}

/* =============================================================================
   STUDY TABLE - MAIN CONTAINER
   ============================================================================= */
.study-table {
  table-layout: fixed;
  font-family:
    "Segoe UI",
    system-ui,
    -apple-system,
    sans-serif;
  font-size: 13px;
  border-collapse: separate;
  border-spacing: 0;
}

/* =============================================================================
   TABLE HEADER - COLUMN TITLES
   ============================================================================= */
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
  padding: 12px 8px !important;
  vertical-align: middle;
  position: sticky;
  transition: var(--study-transition);
}

.study-table-title:hover {
  background-color: rgba(59, 130, 246, 0.05);
}

/* Title Container */
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

.active-modality-filter .badge {
  font-size: 12px;
  padding: 6px 10px;
}

.active-modality-filter .bi-x-circle {
  cursor: pointer;
  font-size: 14px;
}

.active-modality-filter .bi-x-circle:hover {
  opacity: 0.8;
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

/* =============================================================================
   TABLE FILTERS ROW
   ============================================================================= */
.study-table-filters {
  background: linear-gradient(180deg, #ffffff 0%, #f8fafc 100%);
  border-bottom: 1px solid #e2e8f0;
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.02);
}

.study-table-filters>th {
  background-color: transparent;
  padding: 8px 6px !important;
  vertical-align: middle;
}

/* Filter Inputs */
input.form-control.study-list-filter {
  margin-top: var(--filter-margin);
  margin-bottom: var(--filter-margin);
  padding: var(--filter-padding);
  border: 1px solid #cbd5e1;
  border-radius: 6px;
  font-size: 13px;
  transition: var(--study-transition);
  background-color: #ffffff;
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

/* Filter Buttons */
button.form-control.study-list-filter,
.filter-button {
  margin-top: var(--filter-margin);
  margin-bottom: var(--filter-margin);
  padding: var(--filter-padding);
  border: 1px solid #cbd5e1 !important;
  border-radius: 6px;
  background-color: #ffffff;
  transition: var(--study-transition);
  font-weight: 500;
}

.filter-button:hover {
  background-color: #f1f5f9;
  border-color: #3b82f6 !important;
  transform: translateY(-1px);
  box-shadow: 0 2px 4px rgba(59, 130, 246, 0.15);
}

.filter-button:active {
  transform: translateY(0);
}

/* Clear Filter Button */
.study-table-filters button[title="Clear filter"] {
  background: linear-gradient(135deg, #ef4444 0%, #dc2626 100%);
  color: white;
  border: none !important;
}

.study-table-filters button[title="Clear filter"]:hover {
  background: linear-gradient(135deg, #dc2626 0%, #b91c1c 100%);
  box-shadow: 0 2px 8px rgba(239, 68, 68, 0.3);
}

/* Search Button States */
.search-button {
  padding-left: 0 !important;
  font-weight: 600;
}

.is-not-searching {
  background: linear-gradient(135deg, #3b82f6 0%, #2563eb 100%) !important;
  border-color: #3b82f6 !important;
  color: white;
}

.is-not-searching:hover {
  background: linear-gradient(135deg, #2563eb 0%, #1d4ed8 100%) !important;
  box-shadow: var(--study-hover-shadow);
}

.is-searching {
  background: linear-gradient(135deg, #8b5cf6 0%, #7c3aed 100%) !important;
  border-color: #8b5cf6 !important;
  color: white;
}

/* Invalid Filter */
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

/* =============================================================================
   TABLE ACTIONS ROW
   ============================================================================= */
.study-table-actions>th {
  background: linear-gradient(180deg, #f8fafc 0%, #f1f5f9 100%) !important;
  border-bottom: 2px solid #e2e8f0;
  padding: 10px 8px;
  vertical-align: middle;
}

.study-table-actions>th>div {
  background-color: transparent !important;
}

/* Bulk Buttons */
.study-list-bulk-buttons {
  margin-top: var(--filter-margin);
  margin-right: 8px;
  display: flex;
  gap: 6px;
}

/* Alerts in Actions Row */
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

/* Select All Checkbox */
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

/* =============================================================================
   STUDY ROWS
   ============================================================================= */
.study-table> :nth-child(odd)>tr>td {
  background-color: #ffffff;
  min-width: 100%;
}

tr.study-row-expanded {
  background: white !important;
}

.study-table> :nth-child(even)>tr>td {
  background-color: #f8fafc;
}

.study-table>tbody>tr:first-child:hover>* {
  background: linear-gradient(90deg,
      rgba(59, 130, 246, 0.08) 0%,
      rgba(59, 130, 246, 0.03) 100%);
  cursor: pointer;
}

/*
.study-table > tbody > tr.study-row-expanded:hover > *,
.study-table > tbody > tr.study-details-expanded:hover > * {
  background-color: #f1f5f9;
}
 */
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

/* =============================================================================
   STUDY DETAILS TABLE
   ============================================================================= */
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

/* =============================================================================
   MODALITY FILTER DROPDOWN
   ============================================================================= */
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
  background: linear-gradient(90deg,
      rgba(59, 130, 246, 0.1) 0%,
      rgba(59, 130, 246, 0.05) 100%);
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
  transition: var(--study-transition);
}

#modality-filter-dropdown button.btn-primary:hover {
  background: linear-gradient(135deg, #2563eb 0%, #1d4ed8 100%);
  box-shadow: 0 4px 8px rgba(59, 130, 246, 0.3);
  transform: translateY(-1px);
}

/* =============================================================================
   MULTI-LABELS FILTER
   ============================================================================= */
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

/* =============================================================================
   RESPONSIVE IMPROVEMENTS
   ============================================================================= */
@media (max-width: 1200px) {
  .study-table {
    font-size: 12px;
  }

  .study-table td {
    padding: 8px 6px;
  }
}

/* =============================================================================
   ACCESSIBILITY IMPROVEMENTS
   ============================================================================= */
.study-table button:focus-visible,
.study-table input:focus-visible {
  outline: 2px solid #3b82f6;
  outline-offset: 2px;
}

/* =============================================================================
   LOADING STATES & ANIMATIONS
   ============================================================================= */
.spinner-border-sm {
  animation: spinner 0.75s linear infinite;
}

@keyframes spinner {
  to {
    transform: rotate(360deg);
  }
}

/* Skeleton loading for empty states */
.study-table tbody tr.loading-skeleton td {
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
   SCROLLBAR STYLING (для webkit браузеров)
   ============================================================================= */
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
  transition: var(--study-transition);
}

.study-table::-webkit-scrollbar-thumb:hover {
  background: linear-gradient(180deg, #64748b 0%, #475569 100%);
}

button.form-control.study-list-filter.btn.filter-button.btn-sm {
  max-width: 36px;
}

/* =============================================================================
   PRINT STYLES
   ============================================================================= */
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

/* =============================================================================
   DARK MODE SUPPORT (если потребуется)
   ============================================================================= */
@media (prefers-color-scheme: dark) {
  /* Можно добавить dark mode стили если понадобится */
}

.study-table-title {
  padding: 0 4px 0 4px !important;
  position: relative;
  top: 6px;
  left: 4px;
}

/* =============================================================================
   VIEW MODE TOGGLE & CONTROLS
   ============================================================================= */

.study-view-mode-toggle-row {
  background: linear-gradient(180deg, #f8fafc 0%, #f1f5f9 100%);
  border-bottom: 2px solid #e2e8f0;
}

.study-view-mode-toggle-row th {
  padding: 12px 16px !important;
}

.view-mode-controls {
  display: flex;
  align-items: center;
  gap: 20px;
  flex-wrap: wrap;
}

/* View Mode Toggle Buttons */
.view-mode-toggle {
  display: flex;

  padding: 4px;

  height: 32px;
}

.view-mode-btn {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 8px 16px;
  border: none;
  border-radius: 8px;
  background: transparent;
  color: #64748b;
  font-size: 13px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.2s ease;
}

.view-mode-btn:hover {
  background: #f1f5f9;
  color: #3b82f6;
}

.view-mode-btn.active {
  background: linear-gradient(135deg, #3b82f6 0%, #2563eb 100%);
  color: white;
  box-shadow: 0 2px 8px rgba(59, 130, 246, 0.3);
}

.view-mode-btn i {
  font-size: 16px;
}

/* Card View Controls */
.card-view-controls {
  display: flex;
  align-items: center;
  gap: 16px;
  flex: 1;
  justify-content: space-between;
}

/* Quick Filters */
.quick-filters {
  display: flex;
  gap: 6px;
}

.quick-filter-btn {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 6px 12px;
  border: 1px solid #cbd5e1;
  border-radius: 6px;
  background: white;
  color: #64748b;
  font-size: 12px;
  font-weight: 500;
  cursor: pointer;
  transition: all 0.2s ease;
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
  box-shadow: 0 2px 6px rgba(59, 130, 246, 0.25);
}

.quick-filter-btn i {
  font-size: 13px;
}

/* Sort Dropdown */
.card-sort-dropdown {
  display: flex;
  align-items: center;
  gap: 8px;
}

.sort-label {
  display: flex;
  align-items: center;
  gap: 4px;
  font-size: 13px;
  font-weight: 600;
  color: #475569;
  margin: 0;
  white-space: nowrap;
}

.sort-label i {
  font-size: 16px;
  color: #3b82f6;
}

.card-sort-select {
  min-width: 180px;
  border: 1px solid #cbd5e1;
  border-radius: 6px;
  padding: 6px 12px;
  font-size: 13px;
  font-weight: 500;
  color: #334155;
  cursor: pointer;
  transition: all 0.2s ease;
}

.card-sort-select:focus {
  border-color: #3b82f6;
  box-shadow: 0 0 0 3px rgba(59, 130, 246, 0.1);
  outline: none;
}

/* =============================================================================
   CARDS CONTAINER & GRID
   ============================================================================= */

.studies-cards-container {
  padding: 20px;
  background: linear-gradient(180deg, #f8fafc 0%, #ffffff 100%);
  min-height: 400px;
}

.studies-cards-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(300px, 1fr));
  gap: 16px;
  animation: fadeIn 0.4s ease;
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

/* =============================================================================
   RESPONSIVE GRID
   ============================================================================= */

@media (max-width: 480px) {
  .studies-cards-grid {
    grid-template-columns: 1fr;
    gap: 16px;
  }

  .view-mode-controls {
    flex-direction: column;
    align-items: stretch;
  }

  .card-view-controls {
    flex-direction: column;
    gap: 12px;
  }

  .quick-filters {
    width: 100%;
    justify-content: space-between;
  }

  .card-sort-dropdown {
    width: auto;
    height:
  }

  .card-sort-select {
    flex: 1;
  }
}

@media (min-width: 481px) and (max-width: 768px) {
  .studies-cards-grid {
    grid-template-columns: repeat(auto-fill, minmax(320px, 1fr));
  }
}

@media (min-width: 769px) and (max-width: 1200px) {
  .studies-cards-grid {
    grid-template-columns: repeat(auto-fill, minmax(330px, 1fr));
  }
}

@media (min-width: 1201px) {
  .studies-cards-grid {
    grid-template-columns: repeat(auto-fill, minmax(300px, 1fr));
  }
}

/* Large screens - allow more cards per row */
@media (min-width: 1600px) {
  .studies-cards-grid {
    grid-template-columns: repeat(auto-fill, minmax(380px, 1fr));
  }
}

.studies-cards-container .dropdown,
.studies-cards-container .btn-group {
  position: static !important;
}

.studies-cards-container .dropdown-menu {
  position: absolute !important;
  z-index: 9999 !important;
}

.studies-cards-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(320px, 1fr));
  gap: 20px;
  animation: fadeIn 0.4s ease;
  /* Важно для корректной работы grid-column: 1 / -1 */
  grid-auto-flow: dense;
}

/* Раскрытая карточка занимает всю ширину */
.studies-cards-grid>.study-card.expanded {
  grid-column: 1 / -1 !important;
}

/* Плавная анимация при раскрытии */
.studies-cards-grid>.study-card {
  transition: all 0.3s ease;
}

/* =============================================================================
   ИСПРАВЛЕНИЕ ВНУТРЕННЕГО КОНТЕНТА DETAILS
   ============================================================================= */

/* Переопределяем стили для таблиц внутри карточек */
.studies-cards-grid .study-card .details-content {
  width: 100%;
  min-width: 0;
  overflow-x: auto;
}

/* Таблицы внутри details должны отображаться нормально */
.studies-cards-grid .study-card .details-content table {
  width: 100%;
  table-layout: auto;
}

.studies-cards-grid .study-card .details-content td,
.studies-cards-grid .study-card .details-content th {
  white-space: normal;
  word-wrap: break-word;
}

/* Flex контейнеры внутри details */
.studies-cards-grid .study-card .details-content>div {
  width: 100%;
}

/* =============================================================================
   STUDY DETAILS TABLE - ИСПРАВЛЕНИЕ LAYOUT
   ============================================================================= */

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

/* =============================================================================
   SERIES LIST - ИСПРАВЛЕНИЕ LAYOUT
   ============================================================================= */

.studies-cards-grid .series-table {
  width: 100%;
  table-layout: auto;
}

.studies-cards-grid .series-table td,
.studies-cards-grid .series-table th {
  white-space: nowrap;
  padding: 6px 10px;
}

/* =============================================================================
   RESPONSIVE FIXES
   ============================================================================= */

@media (max-width: 768px) {
  .studies-cards-grid {
    grid-template-columns: 1fr;
  }

  .studies-cards-grid .study-card .details-content {
    flex-direction: column;
  }

  .studies-cards-grid .study-details-table {
    font-size: 12px;
  }
}

@media (max-width: 480px) {
  .studies-cards-grid {
    gap: 12px;
    padding: 12px;
  }
}

/* =============================================================================
STUDY TABLE ACTIONS - АДАПТИВНЫЙ LAYOUT=============================================================================*/

/* Сброс старых стилей для study-table-actions */
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

/* Фиксированный левый блок: checkbox */
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

/* Адаптивная середина: все контролы */
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

/* Левая группа: фильтры (только card view) */
.center-left-group {
  display: flex;
  align-items: center;
  gap: 8px;
  flex-shrink: 0;
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
  background: #3b82f60d
}

/* Блок кнопок действий - без отступов, слева от алертов */
.action-buttons-group {
  display: flex;
  align-items: center;
  flex-shrink: 0;
  margin-right: auto;
  /* Прижимает к левой части, алерты к центру */
}

/* Сброс отступов для кнопок внутри ResourceButtonGroup */
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

/* Центральная часть: алерты - в центре */
.alerts-container {
  flex: 0 1 auto;
  min-width: 200px;
  max-width: 400px;
  margin: 0 auto;
}

.alerts-container .study-list-alert {
  margin: 0;
  padding: 8px 12px;
  font-size: 12px;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  border: none;
  border-radius: 6px;
  display: flex;
  align-items: center;
  gap: 8px;
}

/* Кнопка поиска - справа от алертов */
.search-button-wrapper {
  flex-shrink: 0;
  margin-left: auto;
  /* Прижимает к правому краю */
}

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

.search-button.is-searching {
  background: linear-gradient(135deg, #8b5cf6 0%, #7c3aed 100%) !important;
  color: white;
}

/* Фиксированный правый блок: view switcher */
.actions-fixed-right {
  width: 80px;
  min-width: 80px;
  white-space: nowrap;
}

.view-mode-toggle {
  display: flex;
  gap: 4px;
  justify-content: flex-end;
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
}

.view-mode-btn.active {
  background: linear-gradient(135deg, #3b82f6 0%, #2563eb 100%);
  color: white;
}

.studies-cards-grid {
  transition: .5s;
}

/* =============================================================================
   АДАПТИВНОСТЬ - МЕДИА-ЗАПРОСЫ
   ============================================================================= */

@media (max-width: 1200px) {
  .adaptive-controls-wrapper {
    gap: 8px;
  }

  .center-left-group {
    gap: 6px;
  }

  .quick-filter-btn {
    padding: 5px 8px;
  }

  .alerts-container {
    max-width: 300px;
    min-width: 150px;
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

  .alerts-container {
    max-width: 250px;
    min-width: 120px;
  }
}

@media (max-width: 768px) {

  .study-table-actions>th {
    display: n;
    width: 100% !important;
    box-sizing: border-box;
  }

  .quick-filter-btn i {
    font-size: 24px;
  }

  .actions-fixed-left,
  .actions-fixed-right {
    display: inline-block !important;
    width: 50% !important;
  }

  .actions-fixed-left {
    display: none !important;
    width: 50% !important;
  }

  .actions-fixed-right {
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

  .alerts-container {
    order: 3;
    width: 100%;
    max-width: none;
    margin: 8px 0;
    flex: 1 1 100%;
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
    width: 52x;
  }
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


.form-check:hover {
  transform: scale(1.1);
}

button.view-mode-btn:hover {
  transform: scale(1.1);
}

.btn-secondary.btn-icon-small:hover {
  transform: scale(1.05);
}

/* =============================================================================
   MOBILE PORTRAIT STYLES
   ============================================================================= */

@media (max-width: 768px) and (orientation: portrait) {

  /* Study table actions - flex layout */
  tr.study-table-actions {
    display: flex !important;
    flex-wrap: wrap;
    flex-direction: row;
    align-content: center;
    justify-content: center;
    align-items: flex-start;
    width: 100vw;
  }

  /* Header sticky */
  thead.sticky-top {
    display: flex;
    flex-direction: row;
    flex-wrap: wrap;
    align-content: center;
    justify-content: center;
  }

  /* Hide column titles on mobile */
  tr.study-column-titles {
    display: none !important;
  }

  /* Hide filters row on mobile */
  tr.study-table-filters {
    display: none !important;
  }

  /* Hide view mode toggle on mobile (always card view) */
  th.actions-fixed-right {
    display: none !important;
  }

  /* Cards grid - smaller cards on mobile */
  .studies-cards-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(200px, 1fr));
    grid-auto-flow: dense;
    gap: 8px;
    padding: 8px;
  }

  /* Adjust cards container padding */
  .studies-cards-container {
    padding: 8px;
  }

  /* Actions adaptive center - full width */
  .actions-adaptive-center {
    width: 100% !important;
  }

  /* Adaptive controls - column layout on mobile */
  .adaptive-controls-wrapper {
    flex-direction: column;
    gap: 8px;
    padding: 8px 0;
  }

  /* Center left group - full width */
  .center-left-group {
    width: 100%;
    justify-content: space-between;
    flex-wrap: wrap;
  }

  /* Action buttons - center */
  .action-buttons-group {
    margin: 0 auto;
  }

  /* Alerts container - full width */
  .alerts-container {
    width: 100%;
    max-width: none;
    margin: 0;
  }

  /* Search button wrapper */
  .search-button-wrapper {
    margin: 0 auto;
  }

  /* Quick filters - smaller on mobile */
  .quick-filters {
    gap: 4px;
  }

  .quick-filter-btn {
    padding: 6px 8px;
    font-size: 12px;
  }

  /* Fixed left (checkbox) - adjust */
  .actions-fixed-left {
    width: auto !important;
    min-width: auto !important;
  }

  .actions-fixed-left .form-check {
    margin-left: 8px !important;
  }
}

/* Additional mobile-only styles (both orientations) */
@media (max-width: 768px) {
  .studies-cards-grid {
    grid-template-columns: repeat(auto-fill, minmax(200px, 1fr));
    gap: 8px;
  }
}
</style>
