<script>
import StudyItem from "./StudyItem.vue";
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
    placeholder: "1234",
    isOrderable: true,
  },
  PatientID: {
    width: "11%",
    placeholder: "1234",
    isOrderable: true,
  },
  PatientName: {
    width: "20%",
    placeholder: "John^Doe",
    isOrderable: true,
  },
  PatientBirthDate: {
    width: "7%",
    isOrderable: true,
  },
  StudyDescription: {
    width: "15%",
    placeholder: "Chest",
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
      filterStudyDate: "",
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
      multiLabelsComponentKey: 0, // to force refresh the multi-labels filter component
    };
  },
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
      canShowStudiesWithoutLabels: (state) =>
        state.labels.canShowStudiesWithoutLabels,
    }),
    ...mapGetters([
      "studies/isFilterEmpty", // -> this['studies/isFilterEmpty']
      "studies/isMostRecentOrdering", // -> this['studies/isMostRecentOrdering']
    ]),
    notShowingAllResults() {
      if (
        this.sourceType == SourceType.LOCAL_ORTHANC &&
        !this.hasExtendedFind
      ) {
        if (this.studiesIds.length >= this.statistics.CountStudies) {
          return false;
        }
        return this.studiesIds.length == this.uiOptions.MaxStudiesDisplayed; // in this case, the result has been limited
      } else {
        return false;
      }
    },
    isDisplayingMostRecentStudies() {
      return this.status == Status.DISPLAYING_MOST_RECENT_STUDIES;
    },
    isLoadingMostRecentStudies() {
      return this.status == Status.LOADING_MOST_RECENT_STUDIES;
    },
    isDarkMode() {
      // hack to switch the theme: get the value from our custom css
      let bootstrapTheme =
        document.documentElement.getAttribute("data-bs-theme"); // for production
      bootstrapTheme = getComputedStyle(
        document.documentElement,
      ).getPropertyValue("--bootstrap-theme"); // for dev
      // console.log("DatePicker color mode is ", bootstrapTheme);
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
      } else {
        return false;
      }
    },
    isSearchButtonEnabled() {
      if (this.sourceType == SourceType.LOCAL_ORTHANC) {
        return this.uiOptions.StudyListSearchMode == "search-button";
      } else {
        return true;
      }
    },
    showEmptyStudyListIfNoSearch() {
      if (this.sourceType == SourceType.LOCAL_ORTHANC) {
        return this.uiOptions.StudyListContentIfNoSearch == "empty";
      } else {
        return true;
      }
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
      } else {
        return "";
      }
    },
    colSpanBeforeMultiLabelsFilter() {
      let span = 1; // the select study col

      if (this.hasPrimaryViewerIcon) {
        span++;
      }
      if (this.hasPdfReportIcon) {
        span++;
      }
      return span;
    },
    colSpanMultiLabelsFilter() {
      if (this.uiOptions && this.uiOptions.StudyListColumns) {
        let totalColumnsCount = this.uiOptions.StudyListColumns.length + 1; // +1 for selection box
        if (this.hasPrimaryViewerIcon) {
          totalColumnsCount++;
        }
        if (this.hasPdfReportIcon) {
          totalColumnsCount++;
        }
        return (
          totalColumnsCount -
          this.colSpanBeforeMultiLabelsFilter -
          this.colSpanAfterMultiLabelsFilter
        );
      } else {
        return 4; // temporary until the uiOptions have been loaded
      }
    },
    colSpanAfterMultiLabelsFilter() {
      return 3;
    },
    widthColum1() {
      if (this.colSpanClearFilter == 1) {
        return "4%";
      } else {
        return "2%";
      }
    },
    colSpanClearFilter() {
      if (this.sourceType != SourceType.LOCAL_ORTHANC) {
        return 1;
      }

      let span = 1;
      if (this.hasPrimaryViewerIcon) {
        span++;
      }
      if (this.hasPdfReportIcon) {
        span++;
      }

      return span;
    },
  },
  watch: {
    $route: async function () {
      // the watch is used when, e.g, clicking on the back button
      if (!this.updatingRouteWithoutReload) {
        this.updateFilterFromRoute(this.$route.query);
      }
    },
    isConfigurationLoaded(newValue, oldValue) {
      // this is called when opening the page (with a filter or not)
      // console.log("StudyList: Configuration has been loaded, updating study filter: ", this.$route.params.filters);
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
      }, 300); // this is a Hack to prevent this kind of error https://github.com/vuejs/core/issues/5657
    },
    filterModalities: {
      handler(newValue, oldValue) {
        if (!this.updatingFilterUi && !this.initializingModalityFilter) {
          if (this.isSearchAsYouTypeEnabled) {
            //    console.log("StudyList: filterModalities watcher", newValue, oldValue);
            this.updateFilter(
              "ModalitiesInStudy",
              this.getModalityFilter(),
              null,
            );
          } else {
            this.getModalityFilter(); // to update all/none status
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
      // console.log("watch filterStudyDate", newValue);
      this.updateFilter("StudyDate", newValue, oldValue);
    },
    filterStudyDateForDatePicker(newValue, oldValue) {
      let dicomNewValue = dateHelpers.dicomDateFromDatePicker(newValue);
      if (dicomNewValue == null) {
        dicomNewValue = "";
      }
      // console.log("watch filterStudyDateForDatePicker", newValue, dicomNewValue);
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
      // console.log("watch filterPatientBirthDateForDatePicker", newValue, dicomNewValue);
      this.filterPatientBirthDate = dicomNewValue;
    },
    async multiLabelsFilterLabelsConstraint(newValue, oldValue) {
      if (!this.updatingFilterUi) {
        if (this.isSearchAsYouTypeEnabled) {
          if (multiLabelsFilterLabelsConstraint == "None") {
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
      this.multiLabelsComponentKey++; // force refresh the multi-labels filter component
    },
  },
  async created() {
    this.messageBus.on("language-changed", this.translateDatePicker);
    if (this.isConfigurationLoaded) {
      setTimeout(() => {
        this.showMultiLabelsFilter = true;
      }, 300); // this is a Hack to prevent this kind of error https://github.com/vuejs/core/issues/5657
    }
  },
  async mounted() {
    this.updateSelectAll();
  },
  methods: {
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
        // this is the value before the click
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
        return translateDicomTag(
          this.$i18n.t,
          this.$i18n.te,
          "ModalitiesInStudy",
        );
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
      } else {
        return this.columns["undefined"].width;
      }
    },
    isOrderable(tagName) {
      if (
        this.sourceType != SourceType.LOCAL_ORTHANC ||
        !this.hasExtendedFind
      ) {
        return false;
      }

      if (tagName in document._studyColumns) {
        return document._studyColumns[tagName].isOrderable;
      } else {
        return false;
      }
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
        // this tag is already used for ordering -> change order direction
        this.currentOrderDirection =
          this.currentOrderDirection == "ASC" ? "DESC" : "ASC";
      } else {
        // this tag is not used for ordering
        this.currentOrderByTag = tagName;
        this.currentOrderDirection = "ASC";
      }
      let o = {
        Type: "DicomTag",
        Key: this.currentOrderByTag,
        Direction: this.currentOrderDirection,
      };

      // remove all DICOM Tag orders and insert this one as the first one
      this.filterOrderBy = [o].concat(
        this.filterOrderBy.filter((i) => i["Type"] != "DicomTag"),
      );

      this._updateOrderBy(true);
    },
    clearModalityFilter() {
      // console.log("StudyList: clearModalityFilter", this.updatingFilterUi);
      if (this.uiOptions.StudyListColumns) {
        for (const modality of this.uiOptions.ModalitiesFilter) {
          this.filterModalities[modality] = true;
        }
      }
    },
    initModalityFilter() {
      // console.log("StudyList: initModalityFilter", this.updatingFilterUi);
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

      // console.log("getModalityFilter", this.filterModalities);
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
        return "NONE"; // something that will not match !
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
        // if we are using a "search-button", don't update filter now
        return;
      }

      if (dicomTagName == "ModalitiesInStudy" && oldValue == null) {
        // not text: e.g. modalities in study -> update directly
        this._updateFilter(dicomTagName, newValue);
        return;
      }

      if (newValue.length >= this.uiOptions.StudyListSearchAsYouTypeMinChars) {
        // calls updateFilter only after a delay without any key pressed and if there are enough characters entered
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
        // when deleting filter
        this.searchTimerHandler[dicomTagName] = setTimeout(() => {
          this._updateFilter(dicomTagName, "");
        }, this.uiOptions.StudyListSearchAsYouTypeDelay);
      }
    },
    clipFilter(dicomTagName, value) {
      if (this.isFilterLongEnough(dicomTagName, value)) {
        return value;
      } else {
        return "";
      }
    },
    getMinimalFilterLength(dicomTagName) {
      if (
        [
          "AccessionNumber",
          "PatientName",
          "PatientID",
          "StudyDescription",
        ].indexOf(dicomTagName) != -1
      ) {
        if (this.isSearchAsYouTypeEnabled) {
          return this.uiOptions.StudyListSearchAsYouTypeMinChars;
        }
      } else if (
        ["PatientBirthDate", "StudyDate"].indexOf(dicomTagName) != -1
      ) {
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
        ["seriesCount", "instancesCount", "seriesAndInstancesCount"].indexOf(
          tagName,
        ) == -1
      );
    },
    getFilterPlaceholder(tagName) {
      if (tagName in this.columns && this.columns[tagName].placeholder) {
        return this.columns[tagName].placeholder;
      } else {
        return "search-text";
      }
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
      // console.log("StudyList: updateFilterFromRoute", this.updatingFilterUi, filters);

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
        } else if (
          filterKey == "labels-constraint" &&
          !routeHasShowStudiesWithoutLabels
        ) {
          this.multiLabelsFilterLabelsConstraint = filterValue;
        } else if (filterKey == "order-by") {
          if (this.sourceType == SourceType.LOCAL_ORTHANC) {
            // ignore order-by for remote sources
            this.updateOrderBy(filterValue, false);
            routeHasOrderBy = true;
          }
        } else if (filterKey[0] === filterKey[0].toUpperCase()) {
          // DicomTags starts with a capital letter
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

      await this.updateFilterForm(
        keyValueFilters,
        labelsConstraint,
        this.showStudiesWithoutLabels,
      );

      if (
        this.sourceType == SourceType.LOCAL_ORTHANC ||
        !this["studies/isFilterEmpty"]
      ) {
        // do not reload when we are switching to a remote study list to avoid searching for * on a remote server
        await this.reloadStudyList();
      }

      this.multiLabelsComponentKey++; // force refresh the multi-labels filter component

      await nextTick();
      this.updatingFilterUi = false;
    },
    updateFilterForm(filters, labelsConstraint, showStudiesWithoutLabels) {
      // console.log("StudyList: updateFilterForm", this.updatingFilterUi);
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
          this.filterStudyDateForDatePicker =
            dateHelpers.parseDateForDatePicker(value);
        } else if (key == "PatientBirthDate") {
          this.filterPatientBirthDate = value;
          this.filterPatientBirthDateForDatePicker =
            dateHelpers.parseDateForDatePicker(value);
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
      // console.log("StudyList: emptyFilterForm", this.updatingFilterUi);
      this.filterStudyDate = "";
      this.filterStudyDateForDatePicker = null;
      this.filterPatientBirthDate = "";
      this.multiLabelsFilterLabelsConstraint = "All";
      this.showStudiesWithoutLabels = false;

      this.filterPatientBirthDateForDatePicker = null;
      this.filterGenericTags = {};
      if (this.uiOptions.StudyListColumns) {
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
      }
      this.filterLabels = [];
      this.clearModalityFilter();
    },
    isFilteringOnlyOnLabels() {
      let hasGenericTagFilter = false;
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
          if (
            this.filterGenericTags[tag] &&
            this.filterGenericTags[tag] != ""
          ) {
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
        {
          // update filters with the value of filter controls when we click the search button
          await this.$store.dispatch("studies/clearFilterNoReload");
          for (const tag of this.uiOptions.StudyListColumns) {
            if (
              [
                "modalities",
                "seriesCount",
                "instancesCount",
                "seriesAndInstancesCount",
              ].indexOf(tag) == -1
            ) {
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
        }
        await this.updateUrlNoReload();
        await this.reloadStudyList();
      }
    },
    async clearFilters() {
      // console.log("StudyList: clearFilters", this.updatingFilterUi);
      await this.clearFiltersUi();
      await this.$store.dispatch("studies/clearFilterNoReload");

      if (this.sourceType == SourceType.LOCAL_ORTHANC) {
        this.reloadStudyList();
      }
    },
    async clearFiltersUi() {
      // console.log("StudyList: clearFiltersUi IN");
      this.updatingFilterUi = true;

      this.emptyFilterForm();
      this.updateUrl();
      await nextTick();

      this.updatingFilterUi = false;
      // console.log("StudyList: clearFiltersUi OUT");
    },
    async toggleModalityFilter(ev) {
      // only for all/none, other values are binded with v-model !
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

      this.getModalityFilter(); // to update the state of "all"/"none"
    },
    modalityFilterClicked(ev) {
      // prevent closing the drop-down at every click
      ev.stopPropagation();
    },
    closeModalityFilter(ev) {
      // simulate a click on the dropdown toggle (TODO: fix error in console)
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

      if (
        this.filterOrderBy.length > 0 &&
        this.sourceType == SourceType.LOCAL_ORTHANC
      ) {
        let orders = [];
        for (let order of this.filterOrderBy) {
          orders.push(
            [order["Type"], order["Key"], order["Direction"]].join(","),
          );
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
        // if we are displaying most recent studies and there is only a label filter -> continue to show the list of most recent studies (filtered by label)
        const shouldShowMostRecentsWithLabel =
          this.uiOptions.StudyListContentIfNoSearch == "most-recents" &&
          this.isFilteringOnlyOnLabels();

        if (
          this.sourceType == SourceType.LOCAL_ORTHANC &&
          (this["studies/isFilterEmpty"] || shouldShowMostRecentsWithLabel)
        ) {
          await this.$store.dispatch("studies/clearStudies");
          if (this.uiOptions.StudyListContentIfNoSearch == "empty") {
            return;
          } else if (
            this.uiOptions.StudyListContentIfNoSearch == "most-recents" &&
            this.hasExtendedFind
          ) {
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
          } else if (
            this.uiOptions.StudyListContentIfNoSearch == "most-recents"
          ) {
            // legacy code

            if (this.status == Status.LOADING_MOST_RECENT_STUDIES) {
              // if currently loading, stop it
              this.shouldStopLoadingMostRecentStudies = true;
              this.status = Status.DISPLAYING_MOST_RECENT_STUDIES;
            }
            // restart loading
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
        changesResponse = await api.getChangesExtended(toChangeId, limit, [
          "NewStudy",
          "StableStudy",
        ]);
        changes = changesResponse["Changes"];
      } else {
        changesResponse = await api.getChanges(toChangeId - limit, limit);
        changes = changesResponse["Changes"].reverse();
      }

      for (let change of changes) {
        // Take the first event we find -> we see last uploaded data immediately (NewStudy but no StableStudy).
        // An updated study that has received a new series is visible as well (its NewStudy might be too old but the StableStudy brings it back on top of the list)
        if (
          (change["ChangeType"] == "NewStudy" ||
            change["ChangeType"] == "StableStudy") &&
          !this.mostRecentStudiesIds.has(change["ID"])
        ) {
          if (this.shouldStopLoadingMostRecentStudies) {
            return;
          }
          //console.log(change);
          try {
            const study = await api.getStudy(change["ID"]);
            if (
              this.filterLabels.length == 0 ||
              this.filterLabels.filter((l) => study["Labels"].includes(l))
                .length > 0
            ) {
              this.$store.dispatch("studies/addStudy", {
                studyId: change["ID"],
                study: study,
                reloadStats: false,
              });
            }

            this.mostRecentStudiesIds.add(change["ID"]);
            if (
              this.mostRecentStudiesIds.size ==
              this.uiOptions.MaxStudiesDisplayed
            ) {
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
                this.loadStudiesFromChange(
                  Math.max(0, toChangeId - 1000),
                  1000,
                );
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
        if (studyId == this.studiesIds[this.studiesIds.length - 1]) {
          // console.log("Last element shown -> should load more studies");
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
  },
  components: { StudyItem, ResourceButtonGroup, LabelsEditor, Toasts },
};
</script>

<template>
  <div>
    <div
      v-if="isRemoteDicom || isRemoteDicomWeb"
      class="remote-browsing-warning"
    >
      <div>
        <p
          v-if="isRemoteDicom"
          v-html="$t('remote_dicom_browsing', { source: remoteSource })"
        ></p>
        <p
          v-if="isRemoteDicomWeb"
          v-html="$t('remote_dicom_web_browsing', { source: remoteSource })"
        ></p>
      </div>
    </div>
    <table class="table table-sm study-table table-borderless">
      <thead class="sticky-top">
        <tr class="study-column-titles">
          <th :width="widthColum1" max-width="40px" scope="col"></th>
          <th
            v-if="hasPrimaryViewerIcon"
            width="3%"
            max-width="30px"
            scope="col"
          ></th>
          <th
            v-if="hasPdfReportIcon"
            width="3%"
            max-width="30px"
            scope="col"
          ></th>
          <th
            v-for="columnTag in uiOptions.StudyListColumns"
            :key="columnTag"
            data-bs-toggle="tooltip"
            v-bind:title="columnTooltip(columnTag)"
            v-bind:width="columnWidth(columnTag)"
            class="study-table-title"
          >
            <div class="title-container">
              <div
                v-if="isOrderable(columnTag)"
                class="title-text is-orderable"
                @click="toggleOrder($event, columnTag)"
              >
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
            <button
              @click="clearFilters"
              type="button"
              class="form-control study-list-filter btn filter-button btn-sm"
              data-bs-toggle="tooltip"
              title="Clear filter"
            >
              <i class="fa-regular fa-circle-xmark"></i>
            </button>
          </th>
          <th v-for="columnTag in uiOptions.StudyListColumns" :key="columnTag">
            <div v-if="columnTag == 'StudyDate'">
              <Datepicker
                v-if="columnTag == 'StudyDate'"
                v-model="filterStudyDateForDatePicker"
                :enable-time-picker="false"
                range
                :preset-dates="datePickerPresetRanges"
                :format="datePickerFormat"
                :preview-format="datePickerFormat"
                text-input
                arrow-navigation
                hide-input-icon
                :highlight="{ weekdays: [6, 0] }"
                :dark="isDarkMode"
              >
                <template #yearly="{ label, range, presetDate }">
                  <span @click="presetDate(range)">{{ label }}</span>
                </template>
              </Datepicker>
            </div>
            <div v-else-if="columnTag == 'modalities'" class="dropdown">
              <button
                type="button"
                class="btn btn-default btn-sm filter-button dropdown-toggle"
                data-bs-toggle="dropdown"
                id="dropdown-modalities-button"
                aria-expanded="false"
              >
                <span class="fa fa-list"></span>&nbsp;<span
                  class="caret"
                ></span>
              </button>
              <ul
                class="dropdown-menu"
                aria-labelledby="dropdown-modalities-button"
                @click="modalityFilterClicked"
                id="modality-filter-dropdown"
              >
                <li>
                  <label class="dropdown-item"
                    ><input
                      type="checkbox"
                      data-value="all"
                      @click="toggleModalityFilter"
                      v-model="allModalities"
                    />&nbsp;{{ $t("all_modalities") }}</label
                  >
                </li>
                <li>
                  <label class="dropdown-item"
                    ><input
                      type="checkbox"
                      data-value="none"
                      @click="toggleModalityFilter"
                      v-model="noneModalities"
                    />&nbsp;{{ $t("no_modalities") }}</label
                  >
                </li>
                <li>
                  <hr class="dropdown-divider" />
                </li>
                <li
                  v-for="modality in uiOptions.ModalitiesFilter"
                  :key="modality"
                >
                  <label class="dropdown-item"
                    ><input
                      type="checkbox"
                      v-bind:data-value="modality"
                      v-model="filterModalities[modality]"
                    />&nbsp;{{ modality }}</label
                  >
                </li>
                <li>
                  <button
                    class="btn btn-primary mx-5"
                    @click="closeModalityFilter"
                  >
                    {{ $t("close") }}
                  </button>
                </li>
              </ul>
            </div>
            <div v-else-if="columnTag == 'PatientBirthDate'">
              <Datepicker
                v-model="filterPatientBirthDateForDatePicker"
                :enable-time-picker="false"
                range
                :format="datePickerFormat"
                hide-input-icon
                :preview-format="datePickerFormat"
                text-input
                arrow-navigation
                :highlight="{ weekdays: [6, 0] }"
                :dark="isDarkMode"
              >
              </Datepicker>
            </div>
            <input
              v-else-if="hasFilter(columnTag)"
              type="text"
              class="form-control study-list-filter"
              v-model="this.filterGenericTags[columnTag]"
              v-bind:placeholder="getFilterPlaceholder(columnTag)"
              v-bind:class="getFilterClass(columnTag)"
            />
          </th>
        </tr>

        <tr v-if="isMultiLabelsFilterVisible" class="study-table-actions">
          <th :colspan="colSpanBeforeMultiLabelsFilter" scope="col">
            <div class="w-100 d-flex justify-content-end">
              <label class="form-check-label text-end" for="multiLabelsFilter"
                >{{ $t("labels.study_details_title") }}
              </label>
            </div>
          </th>
          <th :colspan="colSpanMultiLabelsFilter" scope="col">
            <LabelsEditor
              id="multiLabelsFilter"
              :labels="filterLabels"
              :key="multiLabelsComponentKey"
              :studyId="null"
              @labelsUpdated="onMultiLabelsFilterChanged"
              :showTitle="false"
              :isFilter="true"
            ></LabelsEditor>
          </th>
          <th :colspan="colSpanAfterMultiLabelsFilter" scope="col">
            <div class="w-100 d-flex">
              <input
                class="form-check-input ms-2 me-1"
                type="radio"
                name="multiLabelsFilterAll"
                id="multiLabelsFilterAll"
                value="All"
                v-model="multiLabelsFilterLabelsConstraint"
              />
              <label class="form-check-label" for="multiLabelsFilterAll"
                >{{ $t("labels.filter_labels_constraint_all") }}
              </label>
              <input
                class="form-check-input ms-2 me-1"
                type="radio"
                name="multiLabelsFilterAny"
                id="multiLabelsFilterAny"
                value="Any"
                v-model="multiLabelsFilterLabelsConstraint"
              />
              <label class="form-check-label" for="multiLabelsFilterAny"
                >{{ $t("labels.filter_labels_constraint_any") }}
              </label>
              <input
                v-if="canShowStudiesWithoutLabels"
                class="form-check-input ms-2 me-1"
                type="radio"
                name="multiLabelsFilterWithoutLabels"
                id="multiLabelsFilterWithoutLabels"
                value="None"
                v-model="multiLabelsFilterLabelsConstraint"
              />
              <label
                v-if="canShowStudiesWithoutLabels"
                class="form-check-label"
                for="multiLabelsFilterWithoutLabels"
                >{{ $t("labels.studies_without_labels") }}
              </label>
            </div>
          </th>
        </tr>
        <tr class="study-table-actions">
          <th width="2%">
            <div class="form-check" style="margin-left: 0.5rem">
              <input
                class="form-check-input"
                type="checkbox"
                v-model="allSelected"
                :indeterminate="isPartialSelected"
                @click="clickSelectAll"
              /><span style="font-weight: 400; font-size: small">{{
                selectedStudiesCount
              }}</span>
            </div>
          </th>
          <th
            width="98%"
            :colspan="colSpanMultiLabelsFilter + colSpanAfterMultiLabelsFilter"
            scope="col"
          >
            <div class="container px-0">
              <div class="row g-1">
                <div class="col-6 study-list-bulk-buttons">
                  <ResourceButtonGroup
                    :resourceLevel="'bulk'"
                    smallIcons="true"
                  >
                  </ResourceButtonGroup>
                </div>
                <div class="col-4">
                  <div
                    v-if="!isSearching && isLoadingMostRecentStudies"
                    class="alert alert-secondary study-list-alert"
                    role="alert"
                  >
                    <span
                      v-if="isLoadingMostRecentStudies"
                      class="spinner-border spinner-border-sm alert-icon"
                      role="status"
                      aria-hidden="true"
                    ></span
                    >{{ $t("loading_most_recent_studies") }}
                  </div>
                  <div
                    v-else-if="!isSearching && isDisplayingMostRecentStudies"
                    class="alert alert-secondary study-list-alert"
                    role="alert"
                  >
                    <i class="bi bi-exclamation-triangle-fill alert-icon"></i
                    >{{ $t("displaying_most_recent_studies") }}
                  </div>
                  <div
                    v-else-if="!isSearching && notShowingAllResults"
                    class="alert alert-danger study-list-alert"
                    role="alert"
                  >
                    <i class="bi bi-exclamation-triangle-fill alert-icon"></i>
                    {{ $t("not_showing_all_results") }} ! !
                  </div>
                  <div
                    v-else-if="
                      !isSearching &&
                      showEmptyStudyListIfNoSearch &&
                      this['studies/isFilterEmpty']
                    "
                    class="alert alert-warning study-list-alert"
                    role="alert"
                  >
                    <i class="bi bi-exclamation-triangle-fill alert-icon"></i>
                    {{ $t("enter_search") }}
                  </div>
                  <div
                    v-else-if="!isSearching && isStudyListEmpty"
                    class="alert alert-warning study-list-alert"
                    role="alert"
                  >
                    <i class="bi bi-exclamation-triangle-fill alert-icon"></i>
                    {{ $t("no_result_found") }}
                  </div>
                  <div
                    v-else-if="isSearching"
                    class="alert alert-secondary study-list-alert"
                    role="alert"
                  >
                    <span
                      v-if="isSearching"
                      class="spinner-border spinner-border-sm alert-icon"
                      role="status"
                      aria-hidden="true"
                    ></span
                    >{{ $t("searching") }}
                  </div>
                </div>
                <div class="col-2">
                  <button
                    @click="search"
                    v-if="isSearchButtonEnabled"
                    type="submit"
                    class="form-control study-list-filter btn filter-button btn-secondary search-button"
                    data-bs-toggle="tooltip"
                    :class="{
                      'is-searching': isSearching,
                      'is-not-searching': !isSearching,
                    }"
                    title="Search"
                  >
                    <i
                      v-if="!isSearching"
                      class="fa-solid fa-magnifying-glass"
                    ></i>
                    <span
                      v-if="isSearching"
                      class="spinner-border spinner-border-sm"
                      role="status"
                      aria-hidden="true"
                    ></span>
                  </button>
                </div>
              </div>
            </div>
          </th>
        </tr>
      </thead>
      <StudyItem
        v-for="studyId in studiesIds"
        :key="studyId"
        :id="studyId"
        :studyId="studyId"
        v-observe-visibility="{ callback: visibilityChanged, once: true }"
        @deletedStudy="onDeletedStudy"
      >
      </StudyItem>
    </table>
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

.study-table-filters > th {
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
.study-table-actions > th {
  background: linear-gradient(180deg, #f8fafc 0%, #f1f5f9 100%) !important;
  border-bottom: 2px solid #e2e8f0;
  padding: 10px 8px;
  vertical-align: middle;
}

.study-table-actions > th > div {
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
  margin-left: 0.5rem;
  display: flex;
  align-items: center;
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
.study-table > :nth-child(odd) > tr > td {
  background-color: #ffffff;
}

.study-table > :nth-child(even) > tr > td {
  background-color: #f8fafc;
}

.study-table > tbody > tr:first-child:hover > * {
  background: linear-gradient(
    90deg,
    rgba(59, 130, 246, 0.08) 0%,
    rgba(59, 130, 246, 0.03) 100%
  );
  cursor: pointer;
}

.study-table > tbody > tr.study-row-expanded:hover > *,
.study-table > tbody > tr.study-details-expanded:hover > * {
  background-color: #f1f5f9;
}

.study-table td {
  text-align: left;
  padding: 12px 10px;
  border-bottom: 1px solid #f1f5f9;
  transition: var(--study-transition);
  font-size: 13px;
  color: #334155;
}

.study-table > :last-child {
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

.study-details-table > :not(caption) > * > *,
.study-details-table > * > * {
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
  background: linear-gradient(
    90deg,
    rgba(59, 130, 246, 0.1) 0%,
    rgba(59, 130, 246, 0.05) 100%
  );
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
</style>
/* =============================================================================
STUDYLIST.VUE - MODERN STYLES (D-MIS Design System) Inspired by SideBar.vue but
adapted for light theme
============================================================================= */
/* CSS Variables */ :root { --filter-margin: 6px; --filter-padding: 8px;
--study-border-radius: 8px; --study-transition: all 0.2s ease;
--study-hover-shadow: 0 2px 8px rgba(59, 130, 246, 0.15); --study-active-shadow:
0 4px 12px rgba(59, 130, 246, 0.25); } /*
=============================================================================
REMOTE BROWSING WARNING
============================================================================= */
.remote-browsing-warning { background: linear-gradient(135deg, #3b82f6 0%,
#8b5cf6 100%); text-align: center; font-weight: 600; height: 2.5rem;
line-height: 2.5rem; color: white; box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
font-family: 'Segoe UI', system-ui, -apple-system, sans-serif; letter-spacing:
0.5px; } .remote-browsing-warning p { margin: 0; padding: 0; } /*
=============================================================================
STUDY TABLE - MAIN CONTAINER
============================================================================= */
.study-table { table-layout: fixed; font-family: 'Segoe UI', system-ui,
-apple-system, sans-serif; font-size: 13px; border-collapse: separate;
border-spacing: 0; } /*
=============================================================================
TABLE HEADER - COLUMN TITLES
============================================================================= */
.study-column-titles { background: linear-gradient(180deg, #f8fafc 0%, #f1f5f9
100%) !important; border-bottom: 2px solid #e2e8f0; font-size: 12px;
font-weight: 600; color: #475569; letter-spacing: 0.3px; text-transform:
uppercase; } .study-table-title { text-align: left; padding: 12px 8px
!important; vertical-align: middle; position: sticky; transition:
var(--study-transition); } .study-table-title:hover { background-color: rgba(59,
130, 246, 0.05); } /* Title Container */ .title-container { position: relative;
width: 100%; height: 2rem; display: flex; align-items: center; } .title-text {
flex: 1; padding: 0 4px; border-right: 1px solid #e2e8f0; text-overflow:
ellipsis; overflow: hidden; white-space: nowrap; transition:
var(--study-transition); } .is-orderable { cursor: pointer; user-select: none;
color: #3b82f6; font-weight: 600; } .is-orderable:hover { color: #2563eb;
transform: translateY(-1px); } .title-arrow { position: absolute; right: 4px;
bottom: 2px; font-size: 16px; color: #3b82f6; animation: arrowPulse 1.5s
ease-in-out infinite; } @keyframes arrowPulse { 0%, 100% { opacity: 1; } 50% {
opacity: 0.6; } } /*
=============================================================================
TABLE FILTERS ROW
============================================================================= */
.study-table-filters { background: linear-gradient(180deg, #ffffff 0%, #f8fafc
100%); border-bottom: 1px solid #e2e8f0; box-shadow: 0 2px 4px rgba(0, 0, 0,
0.02); } .study-table-filters > th { background-color: transparent; padding: 8px
6px !important; vertical-align: middle; } /* Filter Inputs */
input.form-control.study-list-filter { margin-top: var(--filter-margin);
margin-bottom: var(--filter-margin); padding: var(--filter-padding); border: 1px
solid #cbd5e1; border-radius: 6px; font-size: 13px; transition:
var(--study-transition); background-color: #ffffff; }
input.form-control.study-list-filter:focus { border-color: #3b82f6; box-shadow:
0 0 0 3px rgba(59, 130, 246, 0.1); outline: none; }
input.form-control.study-list-filter::placeholder { color: #94a3b8; font-style:
italic; } /* Filter Buttons */ button.form-control.study-list-filter,
.filter-button { margin-top: var(--filter-margin); margin-bottom:
var(--filter-margin); padding: var(--filter-padding); border: 1px solid #cbd5e1
!important; border-radius: 6px; background-color: #ffffff; transition:
var(--study-transition); font-weight: 500; } .filter-button:hover {
background-color: #f1f5f9; border-color: #3b82f6 !important; transform:
translateY(-1px); box-shadow: 0 2px 4px rgba(59, 130, 246, 0.15); }
.filter-button:active { transform: translateY(0); } /* Clear Filter Button */
.study-table-filters button[title="Clear filter"] { background:
linear-gradient(135deg, #ef4444 0%, #dc2626 100%); color: white; border: none
!important; } .study-table-filters button[title="Clear filter"]:hover {
background: linear-gradient(135deg, #dc2626 0%, #b91c1c 100%); box-shadow: 0 2px
8px rgba(239, 68, 68, 0.3); } /* Search Button States */ .search-button {
padding-left: 0 !important; font-weight: 600; } .is-not-searching { background:
linear-gradient(135deg, #3b82f6 0%, #2563eb 100%) !important; border-color:
#3b82f6 !important; color: white; } .is-not-searching:hover { background:
linear-gradient(135deg, #2563eb 0%, #1d4ed8 100%) !important; box-shadow:
var(--study-hover-shadow); } .is-searching { background: linear-gradient(135deg,
#8b5cf6 0%, #7c3aed 100%) !important; border-color: #8b5cf6 !important; color:
white; } /* Invalid Filter */ .is-invalid-filter { border-color: #ef4444
!important; box-shadow: 0 0 0 3px rgba(239, 68, 68, 0.15) !important; animation:
shake 0.3s; } @keyframes shake { 0%, 100% { transform: translateX(0); } 25% {
transform: translateX(-4px); } 75% { transform: translateX(4px); } } /*
=============================================================================
TABLE ACTIONS ROW
============================================================================= */
.study-table-actions > th { background: linear-gradient(180deg, #f8fafc 0%,
#f1f5f9 100%) !important; border-bottom: 2px solid #e2e8f0; padding: 10px 8px;
vertical-align: middle; } .study-table-actions > th > div { background-color:
transparent !important; } /* Bulk Buttons */ .study-list-bulk-buttons {
margin-top: var(--filter-margin); display: flex; gap: 6px; } /* Alerts in
Actions Row */ .study-list-alert { margin-top: var(--filter-margin);
margin-bottom: var(--filter-margin); padding: 8px 12px; border-radius: 6px;
font-size: 12px; font-weight: 500; display: flex; align-items: center; border:
none; box-shadow: 0 1px 3px rgba(0, 0, 0, 0.1); } .alert-secondary { background:
linear-gradient(135deg, #64748b 0%, #475569 100%); color: white; }
.alert-warning { background: linear-gradient(135deg, #f59e0b 0%, #d97706 100%);
color: white; } .alert-danger { background: linear-gradient(135deg, #ef4444 0%,
#dc2626 100%); color: white; } .alert-icon { margin-right: 8px; font-size: 14px;
} /* Select All Checkbox */ .study-table-actions .form-check { margin-left:
0.5rem; display: flex; align-items: center; gap: 8px; } .study-table-actions
.form-check-input { width: 18px; height: 18px; border: 2px solid #cbd5e1;
border-radius: 4px; cursor: pointer; transition: var(--study-transition); }
.study-table-actions .form-check-input:checked { background-color: #3b82f6;
border-color: #3b82f6; } .study-table-actions .form-check-input:hover {
border-color: #3b82f6; box-shadow: 0 0 0 3px rgba(59, 130, 246, 0.1); } /*
=============================================================================
STUDY ROWS
============================================================================= */
.study-table > :nth-child(odd) > tr > td { background-color: #ffffff; }
.study-table > :nth-child(even) > tr > td { background-color: #f8fafc; }
.study-table > tbody > tr:first-child:hover > * { background:
linear-gradient(90deg, rgba(59, 130, 246, 0.08) 0%, rgba(59, 130, 246, 0.03)
100%); cursor: pointer; } .study-table > tbody > tr.study-row-expanded:hover >
*, .study-table > tbody > tr.study-details-expanded:hover > * {
background-color: #f1f5f9; } .study-table td { text-align: left; padding: 12px
10px; border-bottom: 1px solid #f1f5f9; transition: var(--study-transition);
font-size: 13px; color: #334155; } .study-table > :last-child { border-bottom:
2px solid #e2e8f0; } /*
=============================================================================
STUDY DETAILS TABLE
============================================================================= */
.study-details-table { margin-top: var(--details-top-margin); margin-left: 5%;
width: 95% !important; font-size: 0.9rem; border-radius:
var(--study-border-radius); overflow: hidden; box-shadow: 0 4px 6px rgba(0, 0,
0, 0.05); } .study-details-table > :not(caption) > * > *, .study-details-table >
* > * { background: linear-gradient(135deg, #f8fafc 0%, #f1f5f9 100%)
!important; border: 1px solid #e2e8f0; } .study-details-table td {
vertical-align: top; padding: 10px; color: #475569; } .study-details-table th {
font-weight: 600; color: #334155; background: linear-gradient(135deg, #e2e8f0
0%, #cbd5e1 100%) !important; } /*
=============================================================================
MODALITY FILTER DROPDOWN
============================================================================= */
#modality-filter-dropdown { border-radius: 8px; box-shadow: 0 8px 16px rgba(0,
0, 0, 0.15); border: 1px solid #e2e8f0; padding: 8px 0; min-width: 200px; }
#modality-filter-dropdown .dropdown-item { padding: 8px 16px; font-size: 13px;
color: #475569; transition: var(--study-transition); cursor: pointer; }
#modality-filter-dropdown .dropdown-item:hover { background:
linear-gradient(90deg, rgba(59, 130, 246, 0.1) 0%, rgba(59, 130, 246, 0.05)
100%); color: #3b82f6; } #modality-filter-dropdown .dropdown-item
input[type="checkbox"] { width: 16px; height: 16px; margin-right: 8px; cursor:
pointer; accent-color: #3b82f6; } #modality-filter-dropdown .dropdown-divider {
margin: 8px 0; border-color: #e2e8f0; } #modality-filter-dropdown
button.btn-primary { margin: 8px auto; width: calc(100% - 40px); background:
linear-gradient(135deg, #3b82f6 0%, #2563eb 100%); border: none; padding: 8px;
border-radius: 6px; font-weight: 600; transition: var(--study-transition); }
#modality-filter-dropdown button.btn-primary:hover { background:
linear-gradient(135deg, #2563eb 0%, #1d4ed8 100%); box-shadow: 0 4px 8px
rgba(59, 130, 246, 0.3); transform: translateY(-1px); } /*
=============================================================================
MULTI-LABELS FILTER
============================================================================= */
.study-table-actions th label.form-check-label { font-size: 12px; font-weight:
500; color: #475569; cursor: pointer; transition: var(--study-transition); }
.study-table-actions th label.form-check-label:hover { color: #3b82f6; }
.study-table-actions th input[type="radio"] { accent-color: #3b82f6; cursor:
pointer; width: 16px; height: 16px; } /*
=============================================================================
RESPONSIVE IMPROVEMENTS
============================================================================= */
@media (max-width: 1200px) { .study-table { font-size: 12px; } .study-table td {
padding: 8px 6px; } } /*
=============================================================================
ACCESSIBILITY IMPROVEMENTS
============================================================================= */
.study-table button:focus-visible, .study-table input:focus-visible { outline:
2px solid #3b82f6; outline-offset: 2px; } /*
=============================================================================
LOADING STATES & ANIMATIONS
============================================================================= */
.spinner-border-sm { animation: spinner 0.75s linear infinite; } @keyframes
spinner { to { transform: rotate(360deg); } } /* Skeleton loading for empty
states */ .study-table tbody tr.loading-skeleton td { background:
linear-gradient( 90deg, #f1f5f9 25%, #e2e8f0 50%, #f1f5f9 75% );
background-size: 200% 100%; animation: skeleton-loading 1.5s ease-in-out
infinite; } @keyframes skeleton-loading { 0% { background-position: 200% 0; }
100% { background-position: -200% 0; } } /*
=============================================================================
SCROLLBAR STYLING (для webkit браузеров)
============================================================================= */
.study-table::-webkit-scrollbar { width: 8px; height: 8px; }
.study-table::-webkit-scrollbar-track { background: #f1f5f9; border-radius: 4px;
} .study-table::-webkit-scrollbar-thumb { background: linear-gradient(180deg,
#94a3b8 0%, #64748b 100%); border-radius: 4px; transition:
var(--study-transition); } .study-table::-webkit-scrollbar-thumb:hover {
background: linear-gradient(180deg, #64748b 0%, #475569 100%); } /*
=============================================================================
PRINT STYLES
============================================================================= */
@media print { .study-table-filters, .study-table-actions,
.remote-browsing-warning { display: none !important; } .study-table {
box-shadow: none; } .study-table td, .study-table th { border: 1px solid #000
!important; } } /*
=============================================================================
DARK MODE SUPPORT (если потребуется)
============================================================================= */
@media (prefers-color-scheme: dark) { /* Можно добавить dark mode стили если
понадобится */ }
