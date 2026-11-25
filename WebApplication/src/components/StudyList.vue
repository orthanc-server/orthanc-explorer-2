<script>
import StudyItem from "./StudyItem.vue"
import ResourceButtonGroup from "./ResourceButtonGroup.vue"
import LabelsEditor from "./LabelsEditor.vue"
import Toasts from "./Toasts.vue"

import { mapState, mapGetters } from "vuex"
import { baseOe2Url } from "../globalConfigurations"
import { translateDicomTag } from "../locales/i18n"
import resourceHelpers from "../helpers/resource-helpers"
import StudyFilterCell from "./StudyFilterCell.vue"

import { useIsMobileNarrow, useIsMobileWide, useIsMobileAny } from "../helpers/media-helpers.js"
import $ from "jquery"
import api from "../orthancApi";
import { ref } from 'vue';
import SourceType from "../helpers/source-type";
import { ObserveVisibility as vObserveVisibility } from 'vue3-observe-visibility'
import { nextTick } from 'vue'


const Status = Object.freeze({
    UNDEFINED: 0,
    LOADING_MOST_RECENT_STUDIES: 1,
    DISPLAYING_MOST_RECENT_STUDIES: 2,
    DISPLAYING_FILTERED_STUDIES: 3,
});


document._allowedFilters = ["StudyDate", "StudyTime", "AccessionNumber", "PatientID", "PatientName", "PatientBirthDate", "StudyInstanceUID", "StudyID", "StudyDescription", "ModalitiesInStudy", "labels"]

document._studyColumns = {
    "StudyDate": {
        "width": "7%",
        "isOrderable": true
    },
    "AccessionNumber": {
        "width": "11%",
        "placeholder": "1234",
        "isOrderable": true
    },
    "PatientID": {
        "width": "11%",
        "placeholder": "1234",
        "isOrderable": true
    },
    "PatientName": {
        "width": "15%",
        "placeholder": "John^Doe",
        "isOrderable": true
    },
    "PatientBirthDate": {
        "width": "7%",
        "isOrderable": true
    },
    "StudyDescription": {
        "width": "25%",
        "placeholder": "Chest",
        "isOrderable": true
    },
    "modalities": {
        "width": "6%",
        "isOrderable": false
    },
    "seriesCount": {
        "width": "4%",
        "isOrderable": false
    },
    "instancesCount": {
        "width": "4%",
        "isOrderable": false
    },
    "seriesAndInstancesCount": {
        "width": "7%",
        "isOrderable": false
    },
    "undefined": {
        "width": "10%",
        "isOrderable": false
    },
    // columns that are not included by default but that are commonly added
    "PatientSex": {
        "width": "7%",
        "placeholder": "",
        "isOrderable": true
    },
    "OtherPatientIDs": {
        "width": "10%",
        "placeholder": "",
        "isOrderable": true
    },
    "InstitutionName": {
        "width": "12%",
        "placeholder": "",
        "isOrderable": true
    },
    "ReferringPhysician": {
        "width": "10%",
        "placeholder": "",
        "isOrderable": true
    },
    "RequestingPhysician": {
        "width": "10%",
        "placeholder": "",
        "isOrderable": true
    },
    "ManufacturerModelName": {
        "width": "10%",
        "placeholder": "",
        "isOrderable": true
    }

};

export default {
    props: [],
    emits: ['deletedStudy'],
    data() {
        return {
            filterStudyDate: '',
            filterPatientBirthDate: '',
            filterGenericTags: {},
            oldFilterGenericTags: {},
            filterLabels: [],
            currentOrderByTag: null,
            currentOrderDirection: 'ASC',
            filterOrderBy: [{ 'Type': 'Metadata', 'Key': 'LastUpdate', 'Direction': 'DESC' }],
            updatingFilterUi: false,
            updatingRouteWithoutReload: false,
            initializingModalityFilter: false,
            searchTimerHandler: {},
            columns: document._studyColumns,
            mostRecentStudiesIds: [],
            shouldStopLoadingMostRecentStudies: false,
            status: Status.UNDEFINED,
            sourceType: SourceType.LOCAL_ORTHANC,
            remoteSource: null,
            showMultiLabelsFilter: false,
            multiLabelsFilterLabelsConstraint: "All",
            showStudiesWithoutLabels: false,
            multiLabelsComponentKey: 0, // to force refresh the multi-labels filter component
            isMobileAny: useIsMobileAny(),
            isMobileNarrow: useIsMobileNarrow(),
            isMobileWide: useIsMobileWide(),
        };
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
            allLabels: state => state.labels.allLabels,
            isConfigurationLoaded: state => state.configuration.loaded,
            studiesIds: state => state.studies.studiesIds,
            isSearching: state => state.studies.isSearching,
            statistics: state => state.studies.statistics,
            hasExtendedFind: state => state.configuration.hasExtendedFind,
            hasExtendedChanges: state => state.configuration.hasExtendedChanges,
            canShowStudiesWithoutLabels: state => state.labels.canShowStudiesWithoutLabels
        }),
        ...mapGetters([
            'studies/isFilterEmpty',                // -> this['studies/isFilterEmpty']
            'studies/isMostRecentOrdering',         // -> this['studies/isMostRecentOrdering']
        ]),
        allSelected() {
            return this.$store.getters['selection/isStudiesFullSelection'];
        },
        isPartialySelected() {
            return this.$store.getters['selection/isStudiesPartialSelection'];
        },
        notShowingAllResults() {
            if (this.sourceType == SourceType.LOCAL_ORTHANC && !this.hasExtendedFind) {
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
        isRemoteDicom() {
            return this.sourceType == SourceType.REMOTE_DICOM;
        },
        isRemoteDicomWeb() {
            return this.sourceType == SourceType.REMOTE_DICOM_WEB;
        },
        isMultiLabelsFilterVisible() {
            return this.sourceType == SourceType.LOCAL_ORTHANC && this.showMultiLabelsFilter && this.uiOptions.EnableMultiLabelsSearch;
        },
        mobileFilters() {
            if (this.uiOptions.StudyListColumns) {
                return this.uiOptions.StudyListColumns.filter(n => ['seriesAndInstancesCount', 'instancesCount', 'seriesCount'].indexOf(n) == -1);
            } else {
                return [];
            }
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
        datePickerFormat() {  // TO REMOVE
            return this.uiOptions.DateFormat;
        },
        hasPrimaryViewerIcon() {
            return this.sourceType == SourceType.LOCAL_ORTHANC && this.uiOptions.EnableViewerQuickButton;
        },
        hasPdfReportIcon() {
            return this.sourceType == SourceType.LOCAL_ORTHANC && this.uiOptions.EnableReportQuickButton;
        },
        selectedStudiesCount() {
            const count = this.$store.getters['selection/selectedStudiesCount'];
            if (count > 0) {
                return count;
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

            span = Math.max(span, this.colSpanClearFilter + 1);
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
                return totalColumnsCount - this.colSpanBeforeMultiLabelsFilter - this.colSpanAfterMultiLabelsFilter;
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
        }
    },
    watch: {
        '$route': async function () { // the watch is used when, e.g, clicking on the back button
            if (!this.updatingRouteWithoutReload) {
                this.updateFilterFromRoute(this.$route.query);
            }
        },
        isConfigurationLoaded(newValue, oldValue) {
            this.init();
        },
        filterGenericTags: {
            handler(newValue, oldValue) {
                // oldValue is the same as newValue for deep watchers
                for (const [k, v] of Object.entries(this.filterGenericTags)) {
                    let oldValue = null;
                    if (k in this.oldFilterGenericTags) {
                        oldValue = this.oldFilterGenericTags[k]
                    }
                    this.updateFilter(k, v, oldValue);
                    this.oldFilterGenericTags[k] = v;
                }
            },
            deep: true
        },
        async multiLabelsFilterLabelsConstraint(newValue, oldValue) {
            if (!this.updatingFilterUi) {
                if (this.isSearchAsYouTypeEnabled) {
                    if (multiLabelsFilterLabelsConstraint == 'None') {
                        await this.$store.dispatch('studies/updateLabelFilterNoReload', { labels: [], constraint: this.multiLabelsFilterLabelsConstraint });
                    } else {
                        await this.$store.dispatch('studies/updateLabelFilterNoReload', { labels: this.filterLabels, constraint: this.multiLabelsFilterLabelsConstraint });
                    }
                    this.updateUrlNoReload();
                    this.reloadStudyList();
                }
            }
        },
        allLabels(newValue, oldValue) {
            this.multiLabelsComponentKey++; // force refresh the multi-labels filter component
        }
    },
    async created() {
        this.messageBus.on('modalities-filter-updated', this.modalitiesFilterUpdated);
        this.messageBus.on('date-picked', this.datePicked);
        this.messageBus.on('text-filter-updated', this.textFilterUpdated);

        if (this.isConfigurationLoaded) {
            setTimeout(() => { this.showMultiLabelsFilter = true }, 300);  // this is a Hack to prevent this kind of error https://github.com/vuejs/core/issues/5657
        }
    },
    async mounted() {
        if (this.isConfigurationLoaded) { // this happens when we open the app e.g. on the #settings page and then, switch to the study list
            this.init();
        }
        // this.updateSelectAll();
        // this._onDocumentClick = this.onDocumentClick.bind(this);
        // document.addEventListener('click', this._onDocumentClick, true);
    },
    beforeUnmount() {
        document.removeEventListener('click', this._onDocumentClick, true);
    },
    methods: {
        init() {
            console.log('Study List init');
            // this is called when opening the page (with a filter or not)
            // console.log("StudyList: Configuration has been loaded, updating study filter: ", this.$route.params.filters);
            for (const tag of this.uiOptions.StudyListColumns) {
                if (['StudyDate', 'PatientBirthDate', 'modalities', 'seriesCount', 'instancesCount', 'seriesAndInstancesCount'].indexOf(tag) == -1) {
                    this.filterGenericTags[tag] = '';
                }
            }
            this.updateFilterFromRoute(this.$route.query);
            setTimeout(() => { this.showMultiLabelsFilter = true }, 300);  // this is a Hack to prevent this kind of error https://github.com/vuejs/core/issues/5657
        },
        clickSelectAll() {
            if (!this.allSelected) { // this is the value before the click
                this.$store.dispatch('selection/selectAllStudies', { isSelected: true });
            } else {
                this.$store.dispatch('selection/selectAllStudies', { isSelected: false });
            }
        },
        columnTitle(tagName) {
            if (tagName == "seriesCount") {
                return this.$t('series_count_header');
            } else if (tagName == "instancesCount") {
                return this.$t('instances_count_header');
            } else if (tagName == "seriesAndInstancesCount") {
                return this.$t('series_and_instances_count_header');
            } else if (tagName == "modalities") {
                return translateDicomTag("ModalitiesInStudy");
            } else {
                return translateDicomTag(tagName);
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
            if (this.sourceType != SourceType.LOCAL_ORTHANC || !this.hasExtendedFind) {
                return false;
            }

            if (tagName in document._studyColumns) {
                return document._studyColumns[tagName].isOrderable;
            } else {
                return false;
            }
        },
        isOrderTagUp(tagName) {
            return tagName == this.currentOrderByTag && this.currentOrderDirection == 'DESC';
        },
        isOrderTagDown(tagName) {
            return tagName == this.currentOrderByTag && this.currentOrderDirection == 'ASC';
        },
        toggleOrder(ev, tagName) {
            ev.preventDefault();
            ev.stopPropagation();

            if (this.currentOrderByTag == tagName) { // this tag is already used for ordering -> change order direction
                this.currentOrderDirection = (this.currentOrderDirection == 'ASC' ? 'DESC' : 'ASC');
            } else { // this tag is not used for ordering
                this.currentOrderByTag = tagName;
                this.currentOrderDirection = 'ASC';
            }
            let o = { 'Type': 'DicomTag', 'Key': this.currentOrderByTag, 'Direction': this.currentOrderDirection };

            // remove all DICOM Tag orders and insert this one as the first one
            this.filterOrderBy = [o].concat(this.filterOrderBy.filter(i => i['Type'] != 'DicomTag'));

            this._updateOrderBy(true);
        },
        modalitiesFilterUpdated(modalitiesFilter) {
            this.modalitiesFilter = modalitiesFilter;

            if (!this.updatingFilterUi && !this.initializingModalityFilter) {
                if (this.isSearchAsYouTypeEnabled) {
                    //    console.log("StudyList: filterModalities watcher", newValue, oldValue);
                    this.updateFilter('ModalitiesInStudy', this.modalitiesFilter, null);
                }
            }

        },
        textFilterUpdated(message) {
            this.filterGenericTags[message.textTag] = message.textValue;
            this.updateFilter(message.textTag, message.textValue, null);
        },
        datePicked(message) {
            if (message.dateTag == "StudyDate") {
                this.filterStudyDate = message.dateValue;
            } else if (message.dateTag == "PatientBirthDate") {
                this.filterPatientBirthDate = message.dateValue;
            }
            this.updateFilter(message.dateTag, message.dateValue, null);
        },
        updateFilter(dicomTagName, newValue, oldValue) {

            if (this.updatingFilterUi) {
                return;
            }

            if (!this.isSearchAsYouTypeEnabled) { // if we are using a "search-button", don't update filter now
                return;
            }

            if (dicomTagName == "ModalitiesInStudy" && oldValue == null) { // not text: e.g. modalities in study -> update directly
                this._updateFilter(dicomTagName, newValue);
                return;
            }

            if (newValue.length >= this.uiOptions.StudyListSearchAsYouTypeMinChars) {
                // calls updateFilter only after a delay without any key pressed and if there are enough characters entered
                if (this.searchTimerHandler[dicomTagName]) {
                    clearTimeout(this.searchTimerHandler[dicomTagName]);
                }
                this.searchTimerHandler[dicomTagName] = setTimeout(() => { this._updateFilter(dicomTagName, newValue) }, this.uiOptions.StudyListSearchAsYouTypeDelay);
            } else if (oldValue && newValue.length < oldValue.length && oldValue.length >= this.uiOptions.StudyListSearchAsYouTypeMinChars) { // when deleting filter
                this.searchTimerHandler[dicomTagName] = setTimeout(() => { this._updateFilter(dicomTagName, "") }, this.uiOptions.StudyListSearchAsYouTypeDelay);
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
            if (["AccessionNumber", "PatientName", "PatientID", "StudyDescription"].indexOf(dicomTagName) != -1) {
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
            const value = this.getFilterValue(dicomTagName)
            if (value != null && value.length > 0 && !this.isFilterLongEnough(dicomTagName, value)) {
                return "is-invalid-filter";
            }
            return "";
        },
        hasFilter(tagName) {
            return ['seriesCount', 'instancesCount', 'seriesAndInstancesCount'].indexOf(tagName) == -1;
        },
        getFilterId(tagName) {
            return "filter-" + tagName;
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
            this.$store.dispatch('studies/updateFilterNoReload', { dicomTagName: dicomTagName, value: value });
            this.updateUrlNoReload();
            this.reloadStudyList();
        },
        _updateOrderBy(reloadNow) {
            this.$store.dispatch('studies/updateOrderByNoReload', { orderBy: this.filterOrderBy });
            if (reloadNow) {
                this.updateUrlNoReload();
                this.reloadStudyList();
            }
        },
        updateOrderBy(orderString, reloadNow) {
            console.log("updateOrderBy", orderString);
            this.filterOrderBy = [];
            this.currentOrderByTag = null;
            this.currentOrderDirection = 'ASC';

            let orders = orderString.split(';');
            for (let order of orders) {
                let o = order.split(',');

                this.filterOrderBy.push({ 'Type': o[0], 'Key': o[1], 'Direction': o[2] });

                if (o[0] == 'DicomTag' && this.currentOrderByTag == null) {
                    this.currentOrderByTag = o[1];
                    this.currentOrderDirection = o[2];
                }
            }
            this._updateOrderBy(reloadNow);
        },
        async updateFilterFromRoute(filters) {
            // console.log("StudyList: updateFilterFromRoute", this.updatingFilterUi, filters);

            this.updatingFilterUi = true;
            await this.$store.dispatch('studies/clearStudies');
            await this.$store.dispatch('studies/clearFilterNoReload');
            var keyValueFilters = {};

            if ("source-type" in filters && "remote-source" in filters) {
                if (filters["source-type"].toLowerCase() === "dicom") {
                    this.sourceType = SourceType.REMOTE_DICOM;
                } else if (filters["source-type"].toLowerCase() === "dicom-web") {
                    this.sourceType = SourceType.REMOTE_DICOM_WEB;
                }
                this.remoteSource = resourceHelpers.sanitizeToAlphanumeric(filters["remote-source"]);
            } else {
                this.sourceType = SourceType.LOCAL_ORTHANC;
                this.remoteSource = null;
            }
            await this.$store.dispatch('studies/updateSource', { 'source-type': this.sourceType, 'remote-source': this.remoteSource });

            let routeHasOrderBy = false;
            let labelsConstraint = filters["labels-constraint"] || 'All';

            let routeHasShowStudiesWithoutLabels = Object.keys(filters).filter(k => k == 'without-labels').length > 0;

            for (const [filterKey, filterValue] of Object.entries(filters)) {
                if (filterKey == "labels" && !routeHasShowStudiesWithoutLabels) {
                    const labels = filterValue.split(",");
                    keyValueFilters[filterKey] = labels;
                    await this.$store.dispatch('studies/updateLabelFilterNoReload', { labels: labels, constraint: labelsConstraint });
                } else if (filterKey == 'labels-constraint' && !routeHasShowStudiesWithoutLabels) {
                    this.multiLabelsFilterLabelsConstraint = filterValue;
                } else if (filterKey == 'order-by') {
                    if (this.sourceType == SourceType.LOCAL_ORTHANC) { // ignore order-by for remote sources
                        this.updateOrderBy(filterValue, false);
                        routeHasOrderBy = true;
                    }
                } else if (filterKey[0] === filterKey[0].toUpperCase()) {  // DicomTags starts with a capital letter
                    keyValueFilters[filterKey] = filterValue;
                    await this.$store.dispatch('studies/updateFilterNoReload', { dicomTagName: filterKey, value: filterValue });
                }
            }
            if (routeHasShowStudiesWithoutLabels) {
                await this.$store.dispatch('studies/updateLabelFilterNoReload', { labels: [], constraint: 'None' });
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

            if (this.sourceType == SourceType.LOCAL_ORTHANC || !this['studies/isFilterEmpty']) { // do not reload when we are switching to a remote study list to avoid searching for * on a remote server
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
                    let orders = value.split(";")
                    for (let order of orders) {
                        let s = order.split(",");
                    }
                } else if (key == "StudyDate") {
                    this.filterStudyDate = value;
                    this.messageBus.emit('set-date-picker', { dateTag: 'StudyDate', dateValue: value });
                } else if (key == "PatientBirthDate") {
                    this.filterPatientBirthDate = value;
                    this.messageBus.emit('set-date-picker', { dateTag: 'PatientBirthDate', dateValue: value });
                } else if (key == "ModalitiesInStudy") {
                    this.messageBus.emit('update-modalities-filter', value);
                } else {
                    this.filterGenericTags[key] = value;
                    this.messageBus.emit('set-text-filter', { textTag: key, textValue: value });
                }
            }
        },
        emptyFilterForm() {
            // console.log("StudyList: emptyFilterForm", this.updatingFilterUi);
            this.filterStudyDate = '';
            this.filterPatientBirthDate = '';
            this.multiLabelsFilterLabelsConstraint = 'All';
            this.showStudiesWithoutLabels = false;

            this.filterGenericTags = {};
            if (this.uiOptions.StudyListColumns) {
                for (const tag of this.uiOptions.StudyListColumns) {
                    if (['StudyDate', 'PatientBirthDate', 'modalities', 'seriesCount', 'instancesCount', 'seriesAndInstancesCount'].indexOf(tag) == -1) {
                        this.filterGenericTags[tag] = '';
                        this.messageBus.emit("clear-text-filter", { textTag: tag });
                    }
                }
            }
            this.filterLabels = [];
            this.messageBus.emit("clear-modalities-filter");
            this.messageBus.emit("clear-date-picker", { dateTag: "StudyDate" });
            this.messageBus.emit("clear-date-picker", { dateTag: "PatientBirthDate" });

        },
        isFilteringOnlyOnLabels() {
            let hasGenericTagFilter = false;
            for (const tag of this.uiOptions.StudyListColumns) {
                if (['StudyDate', 'PatientBirthDate', 'modalities', 'seriesCount', 'instancesCount', 'seriesAndInstancesCount'].indexOf(tag) == -1) {
                    if (this.filterGenericTags[tag] && this.filterGenericTags[tag] != '') {
                        hasGenericTagFilter = true;
                    }
                }
            }
            return this.filterStudyDate == '' && this.filterPatientBirthDate == '' && !hasGenericTagFilter && this.filterLabels.length > 0 && this.filterOrderBy.length == 0;
        },
        async search() {
            if (this.isSearching) {
                await this.$store.dispatch('studies/cancelSearch');
            } else {
                {
                    // update filters with the value of filter controls when we click the search button
                    await this.$store.dispatch('studies/clearFilterNoReload');
                    for (const tag of this.uiOptions.StudyListColumns) {
                        if (['modalities', 'seriesCount', 'instancesCount', 'seriesAndInstancesCount'].indexOf(tag) == -1) {
                            await this.$store.dispatch('studies/updateFilterNoReload', { dicomTagName: tag, value: this.getFilterValue(tag) });
                        }
                    }
                    await this.$store.dispatch('studies/updateFilterNoReload', { dicomTagName: "ModalitiesInStudy", value: this.modalitiesFilter });
                    if (this.showStudiesWithoutLabels) {
                        await this.$store.dispatch('studies/updateLabelFilterNoReload', { labels: [], constraint: 'None' });
                    } else {
                        await this.$store.dispatch('studies/updateLabelFilterNoReload', { labels: this.filterLabels, constraint: this.multiLabelsFilterLabelsConstraint });
                    }
                }
                await this.updateUrlNoReload();
                await this.reloadStudyList();
            }
        },
        async clearFilters() {
            // console.log("StudyList: clearFilters", this.updatingFilterUi);
            await this.clearFiltersUi();
            await this.$store.dispatch('studies/clearFilterNoReload');

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
        // onDocumentClick(ev) {
        //     if (!this.showModalityDropdown) return;
        //     const dropdown = document.getElementById('modality-filter-dropdown');
        //     let btn = document.getElementById('dropdown-modalities-button');
        //     if (!btn) btn = document.getElementById('dropdown-modalities-button-mobile');
        //     if (!btn) btn = document.getElementById('dropdown-modalities-button-mobile-row2');
        //     if (!btn) btn = document.getElementById('dropdown-modalities-button-mobile-landscape');

        //     if (dropdown && dropdown.contains(ev.target)) return;
        //     if (btn && btn.contains(ev.target)) return;
        //     this.showModalityDropdown = false;
        // },
        async updateUrlNoReload() {
            this.updatingRouteWithoutReload = true;
            await this.updateUrl();
            this.updatingRouteWithoutReload = false;
        },
        async updateUrl() {
            let query = {};

            if (this.sourceType != SourceType.LOCAL_ORTHANC) {
                if (this.sourceType == SourceType.REMOTE_DICOM) {
                    query['source-type'] = 'dicom';
                } else if (this.sourceType == SourceType.REMOTE_DICOM_WEB) {
                    query['source-type'] = 'dicom-web';
                }
                query['remote-source'] = this.remoteSource;
            }

            if (this.clipFilter("StudyDate", this.filterStudyDate)) {
                query['StudyDate'] = this.filterStudyDate;
            }
            if (this.clipFilter("PatientBirthDate", this.filterPatientBirthDate)) {
                query['PatientBirthDate'] = this.filterPatientBirthDate;
            }
            if (this.modalitiesFilter) {
                query['ModalitiesInStudy'] = this.modalitiesFilter;
            }
            for (const [k, v] of Object.entries(this.filterGenericTags)) {
                if (this.clipFilter(k, v)) {
                    query[k] = v;
                }
            }
            if (this.filterLabels.length > 0) {
                query['labels'] = this.filterLabels.join(',');

                if (this.multiLabelsFilterLabelsConstraint != 'All') {
                    query['labels-constraint'] = this.multiLabelsFilterLabelsConstraint;
                }
            }

            if (this.filterOrderBy.length > 0 && this.sourceType == SourceType.LOCAL_ORTHANC) {
                let orders = []
                for (let order of this.filterOrderBy) {
                    orders.push([order['Type'], order['Key'], order['Direction']].join(','))
                }
                query['order-by'] = orders.join(';');
            }

            let newUrl = "/filtered-studies?" + (new URLSearchParams(query)).toString();
            await this.$router.replace(newUrl);
        },
        async extendStudyList() {
            if (this.sourceType == SourceType.LOCAL_ORTHANC && this.hasExtendedFind) {
                await this.$store.dispatch('studies/extendFilteredStudies');
            }
        },
        async reloadStudyList() {
            if (this.sourceType == SourceType.LOCAL_ORTHANC && this.hasExtendedFind) {
                if (this.uiOptions.StudyListContentIfNoSearch == "empty") {
                    this.status = Status.UNDEFINED;
                    if (this['studies/isFilterEmpty']) {
                        await this.$store.dispatch('studies/clearStudies');
                    } else {
                        await this.$store.dispatch('studies/clearStudies');
                        await this.$store.dispatch('studies/reloadFilteredStudies');
                    }
                } else {
                    if (this['studies/isMostRecentOrdering']) {
                        this.status = Status.LOADING_MOST_RECENT_STUDIES;
                    }
                    await this.$store.dispatch('studies/clearStudies');
                    await this.$store.dispatch('studies/reloadFilteredStudies');
                    if (this['studies/isMostRecentOrdering']) {
                        this.status = Status.DISPLAYING_MOST_RECENT_STUDIES;
                    } else {
                        this.status = Status.DISPLAYING_FILTERED_STUDIES;
                    }
                }
            } else {
                // if we are displaying most recent studies and there is only a label filter -> continue to show the list of most recent studies (filtered by label)
                const shouldShowMostRecentsWithLabel = this.uiOptions.StudyListContentIfNoSearch == "most-recents" && this.isFilteringOnlyOnLabels();

                if (this.sourceType == SourceType.LOCAL_ORTHANC && (this['studies/isFilterEmpty'] || shouldShowMostRecentsWithLabel)) {
                    await this.$store.dispatch('studies/clearStudies');
                    if (this.uiOptions.StudyListContentIfNoSearch == "empty") {
                        return;
                    } else if (this.uiOptions.StudyListContentIfNoSearch == "most-recents" && this.hasExtendedFind) {
                        const studies = await api.getMostRecentStudiesExtended((this.filterLabels.length > 0 ? this.filterLabels[0] : null));
                        for (const study of studies) {
                            this.$store.dispatch('studies/addStudy', { studyId: study["ID"], study: study, reloadStats: false });
                        }
                    } else if (this.uiOptions.StudyListContentIfNoSearch == "most-recents") {
                        // legacy code

                        if (this.status == Status.LOADING_MOST_RECENT_STUDIES) {
                            // if currently loading, stop it
                            this.shouldStopLoadingMostRecentStudies = true;
                            this.status = Status.DISPLAYING_MOST_RECENT_STUDIES;
                        }
                        // restart loading 
                        const lastChangeId = await api.getLastChangeId();

                        await this.$store.dispatch('studies/clearStudies');
                        this.mostRecentStudiesIds = new Set();
                        this.shouldStopLoadingMostRecentStudies = false;
                        this.status = Status.LOADING_MOST_RECENT_STUDIES;

                        this.loadStudiesFromChange(lastChangeId, 1000);
                    }
                } else {
                    this.shouldStopLoadingMostRecentStudies = true;
                    this.status = Status.UNDEFINED;
                    await this.$store.dispatch('studies/reloadFilteredStudies');
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
                // Take the first event we find -> we see last uploaded data immediately (NewStudy but no StableStudy).  
                // An updated study that has received a new series is visible as well (its NewStudy might be too old but the StableStudy brings it back on top of the list)
                if ((change["ChangeType"] == "NewStudy" || change["ChangeType"] == "StableStudy") && !this.mostRecentStudiesIds.has(change["ID"])) {
                    if (this.shouldStopLoadingMostRecentStudies) {
                        return;
                    }
                    //console.log(change);
                    try {
                        const study = await api.getStudy(change["ID"]);
                        if (this.filterLabels.length == 0 || this.filterLabels.filter(l => study["Labels"].includes(l)).length > 0) {
                            this.$store.dispatch('studies/addStudy', { studyId: change["ID"], study: study, reloadStats: false });
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
                            setTimeout(() => { this.loadStudiesFromChange(changesResponse["First"], 1000) }, 1);
                        }
                    } else {
                        if (toChangeId != changesResponse["First"]) {
                            setTimeout(() => { this.loadStudiesFromChange(Math.max(0, toChangeId - 1000), 1000) }, 1);
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
            this.$store.dispatch('studies/deleteStudy', { studyId: studyId });
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
        }
    },
    components: { StudyItem, ResourceButtonGroup, LabelsEditor, Toasts, StudyFilterCell }
}
</script>


<template>
    <div>
        <div v-if="isRemoteDicom || isRemoteDicomWeb" class="remote-browsing-warning">
            <div>
                <p v-if="isRemoteDicom" v-html="$t('remote_dicom_browsing', { source: remoteSource })"></p>
                <p v-if="isRemoteDicomWeb" v-html="$t('remote_dicom_web_browsing', { source: remoteSource })"></p>
            </div>
        </div>
        <table v-if="!isMobileAny || isMobileWide" class="table table-sm study-table table-borderless">
            <thead class="sticky-top">
                <tr class="study-column-titles">
                    <th :width="widthColum1" class="col-prefix col-checkbox" scope="col"></th>
                    <th v-if="hasPrimaryViewerIcon" width="2%" class="col-prefix col-viewer" scope="col"></th>
                    <th v-if="hasPdfReportIcon" width="2%" class="col-prefix col-pdf" scope="col"></th>
                    <th v-for="columnTag in uiOptions.StudyListColumns" :key="columnTag" data-bs-toggle="tooltip"
                        v-bind:title="columnTooltip(columnTag)" v-bind:width="columnWidth(columnTag)"
                        class="study-table-title">
                        <div class="title-container">
                            <div v-if="isOrderable(columnTag)" class="title-text is-orderable"
                                @click="toggleOrder($event, columnTag)">{{ columnTitle(columnTag) }}</div>
                            <div v-if="!isOrderable(columnTag)" class="title-text">{{ columnTitle(columnTag) }}</div>
                            <div v-if="isOrderTagUp(columnTag)" class="title-arrow"><i class="bi bi-arrow-up"></i></div>
                            <div v-if="isOrderTagDown(columnTag)" class="title-arrow"><i class="bi bi-arrow-down"></i>
                            </div>
                        </div>
                    </th>
                </tr>
                <tr class="study-table-filters" v-on:keyup.enter="search">
                    <th scope="col" :colspan="colSpanClearFilter">
                        <button @click="clearFilters" type="button"
                            class="form-control study-list-filter btn filter-button btn-sm" data-bs-toggle="tooltip"
                            title="Clear filter">
                            <i class="fa-regular fa-circle-xmark"></i>
                        </button>
                    </th>
                    <th v-for="columnTag in uiOptions.StudyListColumns" :key="columnTag">
                        <div class="dropdown">
                            <StudyFilterCell :columnTag="columnTag" idPrefix="desktop"
                                :placeholder="getFilterPlaceholder(columnTag)"
                                :filterClass="getFilterClass(columnTag)" />
                        </div>
                    </th>
                </tr>

                <tr v-if="isMultiLabelsFilterVisible" class="study-table-actions">
                    <th :colspan="colSpanBeforeMultiLabelsFilter" scope="col">
                        <div class="w-100 d-flex justify-content-end">
                            <label class="form-check-label text-end" for="multiLabelsFilter">{{
                                $t('labels.study_details_title') }}
                            </label>
                        </div>
                    </th>
                    <th :colspan="colSpanMultiLabelsFilter" scope="col">
                        <LabelsEditor id="multiLabelsFilter" :labels="filterLabels" :key="multiLabelsComponentKey"
                            :studyId="null" @labelsUpdated="onMultiLabelsFilterChanged" :showTitle="false"
                            :isFilter="true"></LabelsEditor>
                    </th>
                    <th :colspan="colSpanAfterMultiLabelsFilter" scope="col">
                        <div class="w-100 d-flex">
                            <input class="form-check-input ms-2 me-1" type="radio" name="multiLabelsFilterAll"
                                id="multiLabelsFilterAll" value="All" v-model="multiLabelsFilterLabelsConstraint">
                            <label class="form-check-label" for="multiLabelsFilterAll">{{
                                $t('labels.filter_labels_constraint_all') }}
                            </label>
                            <input class="form-check-input ms-2 me-1" type="radio" name="multiLabelsFilterAny"
                                id="multiLabelsFilterAny" value="Any" v-model="multiLabelsFilterLabelsConstraint">
                            <label class="form-check-label" for="multiLabelsFilterAny">{{
                                $t('labels.filter_labels_constraint_any') }}
                            </label>
                            <input v-if="canShowStudiesWithoutLabels" class="form-check-input ms-2 me-1" type="radio"
                                name="multiLabelsFilterWithoutLabels" id="multiLabelsFilterWithoutLabels" value="None"
                                v-model="multiLabelsFilterLabelsConstraint">
                            <label v-if="canShowStudiesWithoutLabels" class="form-check-label"
                                for="multiLabelsFilterWithoutLabels">{{
                                    $t('labels.studies_without_labels') }}
                            </label>
                        </div>
                    </th>
                </tr>
                <tr class="study-table-actions">
                    <th width="2%" :colspan="colSpanBeforeMultiLabelsFilter" scope="col">
                        <div class="form-check" style="margin-left: 0.5rem">
                            <input class="form-check-input" type="checkbox" :checked="allSelected"
                                :indeterminate="isPartialySelected" @click="clickSelectAll"><span
                                style="font-weight: 400; font-size: small;">{{ selectedStudiesCount
                                }}</span>
                        </div>
                    </th>
                    <th width="98%" :colspan="colSpanMultiLabelsFilter + colSpanAfterMultiLabelsFilter" scope="col">
                        <div class="container px-0">
                            <div class="row g-1">
                                <div class="col-6 study-list-bulk-buttons">
                                    <ResourceButtonGroup :resourceLevel="'bulk'" smallIcons="true">
                                    </ResourceButtonGroup>
                                </div>
                                <div class="col-4">
                                    <div v-if="!isSearching && isLoadingMostRecentStudies"
                                        class="alert alert-secondary study-list-alert" role="alert">
                                        <span v-if="isLoadingMostRecentStudies"
                                            class="spinner-border spinner-border-sm alert-icon" role="status"
                                            aria-hidden="true"></span>{{
                                                $t('loading_most_recent_studies') }}
                                    </div>
                                    <div v-else-if="!isSearching && isDisplayingMostRecentStudies"
                                        class="alert alert-secondary study-list-alert" role="alert">
                                        <i class="bi bi-exclamation-triangle-fill alert-icon"></i>{{
                                            $t('displaying_most_recent_studies') }}
                                    </div>
                                    <div v-else-if="!isSearching && notShowingAllResults"
                                        class="alert alert-danger study-list-alert" role="alert">
                                        <i class="bi bi-exclamation-triangle-fill alert-icon"></i> {{
                                            $t('not_showing_all_results') }} !
                                        !
                                    </div>
                                    <div v-else-if="!isSearching && showEmptyStudyListIfNoSearch && this['studies/isFilterEmpty']"
                                        class="alert alert-warning study-list-alert" role="alert">
                                        <i class="bi bi-exclamation-triangle-fill alert-icon"></i> {{ $t('enter_search')
                                        }}
                                    </div>
                                    <div v-else-if="!isSearching && isStudyListEmpty"
                                        class="alert alert-warning study-list-alert" role="alert">
                                        <i class="bi bi-exclamation-triangle-fill alert-icon"></i> {{
                                            $t('no_result_found') }}
                                    </div>
                                    <div v-else-if="isSearching" class="alert alert-secondary study-list-alert"
                                        role="alert">
                                        <span v-if="isSearching" class="spinner-border spinner-border-sm alert-icon"
                                            role="status" aria-hidden="true"></span>{{
                                                $t('searching') }}
                                    </div>
                                </div>
                                <div class="col-2">
                                    <button @click="search" v-if="isSearchButtonEnabled" type="submit"
                                        class="form-control study-list-filter btn filter-button btn-secondary search-button"
                                        data-bs-toggle="tooltip"
                                        :class="{ 'is-searching': isSearching, 'is-not-searching': !isSearching }"
                                        id="search" title="Search">
                                        <i v-if="!isSearching" class="fa-solid fa-magnifying-glass"></i>
                                        <span v-if="isSearching" class="spinner-border spinner-border-sm" role="status"
                                            aria-hidden="true"></span>
                                    </button>
                                </div>
                            </div>
                        </div>
                    </th>
                </tr>
            </thead>
            <StudyItem v-for="studyId in studiesIds" :key="studyId" :id="'studyItem-' + studyId" :studyId="studyId"
                v-observe-visibility="{ callback: visibilityChanged, once: true }" @deletedStudy="onDeletedStudy">
            </StudyItem>
        </table>
        <div v-else class="mobile-study-list" :class="{ 'mobile-landscape': isMobileWide }">
            <!-- Filter Strip (single row that wraps onto multiple lines on small screens) -->
            <div class="mobile-filter-strip" v-on:keyup.enter="search">
                <div class="mobile-filter-row">
                    <div v-for="columnTag in mobileFilters" :key="columnTag" class="mobile-filter-column">
                        <div class="mobile-filter-title"
                            @click="isOrderable(columnTag) ? toggleOrder($event, columnTag) : null"
                            :class="{ 'is-orderable': isOrderable(columnTag) }">
                            {{ columnTitle(columnTag) }}
                            <div v-if="isOrderTagUp(columnTag)" class="title-arrow"><i class="bi bi-arrow-up"></i>
                            </div>
                            <div v-if="isOrderTagDown(columnTag)" class="title-arrow"><i class="bi bi-arrow-down"></i>
                            </div>
                        </div>
                        <div class="mobile-filter-input">
                            <StudyFilterCell :columnTag="columnTag" idPrefix="mobile"
                                :placeholder="getFilterPlaceholder(columnTag)"
                                :filterClass="getFilterClass(columnTag)" />
                        </div>
                    </div>
                    <!-- Buttons group: must wrap together on small screens -->
                    <div class="mobile-filter-buttons">
                        <!-- Clear button -->
                        <div class="mobile-filter-column mobile-clear-col">
                            <div class="mobile-filter-title">&nbsp;</div>
                            <div class="mobile-filter-input">
                                <button @click="clearFilters" type="button"
                                    class="btn filter-button btn-sm btn-secondary mobile-clear-filter"
                                    title="Clear filter">
                                    <i class="fa-regular fa-circle-xmark"></i>
                                </button>
                            </div>
                        </div>
                        <!-- Search button -->
                        <div class="mobile-filter-column mobile-search-col">
                            <div class="mobile-filter-title">&nbsp;</div>
                            <div class="mobile-filter-input">
                                <button @click="search" type="submit"
                                    class="form-control study-list-filter btn filter-button btn-secondary search-button"
                                    :class="{ 'is-searching': isSearching, 'is-not-searching': !isSearching }"
                                    title="Search">
                                    <i v-if="!isSearching" class="fa-solid fa-magnifying-glass"></i>
                                    <span v-if="isSearching" class="spinner-border spinner-border-sm" role="status"
                                        aria-hidden="true"></span>
                                </button>
                            </div>
                        </div>
                    </div>
                </div>
            </div>

            <!-- Mobile Labels Bar (if enabled) -->
            <div v-if="isMultiLabelsFilterVisible"
                class="mobile-labels-filter d-flex align-items-center justify-content-between my-2 p-2 rounded">
                <!-- Left side: Title and Radios -->
                <div class="d-flex flex-column me-3" style="min-width: 80px;">
                    <div class="text-light mb-1 text-center" style="font-size: 1rem;">{{
                        $t('labels.study_details_title') ||
                        'Labels' }}</div>
                    <div class="d-flex align-items-center justify-content-center">
                        <div class="form-check form-check-inline m-0 p-0 me-3 d-flex align-items-center">
                            <input class="form-check-input m-0 me-1" type="radio" name="mobileMultiLabelsFilter"
                                value="All" v-model="multiLabelsFilterLabelsConstraint"
                                style="width: 14px; height: 14px;">
                            <label class="form-check-label text-light m-0" style="font-size: 0.9rem;">{{
                                $t('labels.filter_labels_constraint_all') || 'All' }}</label>
                        </div>
                        <div class="form-check form-check-inline m-0 p-0 d-flex align-items-center">
                            <input class="form-check-input m-0 me-1" type="radio" name="mobileMultiLabelsFilter"
                                value="Any" v-model="multiLabelsFilterLabelsConstraint"
                                style="width: 14px; height: 14px;">
                            <label class="form-check-label text-light m-0" style="font-size: 0.9rem;">{{
                                $t('labels.filter_labels_constraint_any') || 'Any' }}</label>
                        </div>
                    </div>
                </div>

                <!-- Right side: Dropdown -->
                <div class="flex-grow-1">
                    <LabelsEditor id="multiLabelsFilterMobile" :labels="filterLabels" :key="multiLabelsComponentKey"
                        :studyId="null" @labelsUpdated="onMultiLabelsFilterChanged" :showTitle="false" :isFilter="true">
                    </LabelsEditor>
                </div>
            </div>

            <!-- Action Bar -->
            <div class="mobile-action-bar sticky-top py-2"
                style="background-color: var(--study-table-actions-bg-color); z-index: 10;">
                <div class="d-flex align-items-center">
                    <div class="form-check mx-2">
                        <input class="form-check-input" type="checkbox" v-model="allSelected"
                            :indeterminate="isPartialySelected" @click="clickSelectAll">
                        <span style="font-weight: 500; font-size: 0.9rem;">{{ selectedStudiesCount }}</span>
                    </div>
                    <div class="mobile-bulk-actions mt-2 px-2">
                        <ResourceButtonGroup :resourceLevel="'bulk'" smallIcons="true"></ResourceButtonGroup>
                    </div>
                </div>
            </div>

            <!-- Cards -->
            <div class="resource-cards-container mt-2">
                <StudyItem v-for="studyId in studiesIds" :key="studyId" :id="'studyItem-' + studyId" :studyId="studyId"
                    v-observe-visibility="{ callback: visibilityChanged, once: true }" @deletedStudy="onDeletedStudy">
                </StudyItem>
            </div>
        </div>
        <Toasts />
    </div>
</template>

<style lang="scss">
:root {
    --filter-margin: 5px;
    --filter-padding: 2px;
}

input.form-control.study-list-filter {
    margin-top: var(--filter-margin);
    margin-bottom: var(--filter-margin);
    padding-top: var(--filter-padding);
    padding-bottom: var(--filter-padding);
    padding-left: 4px;
    padding-right: 4px;
    border-bottom-width: thin;
}

.filter-button {
    border-bottom-width: thin !important;
    border-color: var(--bs-border-color);
}

.search-button {
    padding-left: 0px !important;
}

.is-not-searching {
    background-color: var(--table-filters-is-not-searching-color) !important;
    border-color: var(--table-filters-is-not-searching-color) !important;
}

.is-searching {
    background-color: var(--table-filters-is-searching-color) !important;
    border-color: var(--table-filters-is-searching-color) !important;
}

button.form-control.study-list-filter {
    margin-top: var(--filter-margin);
    margin-bottom: var(--filter-margin);
    padding-top: var(--filter-padding);
    padding-bottom: var(--filter-padding);
}


.study-column-titles {
    background-color: var(--study-table-header-bg-color) !important;
    font-size: smaller;
}


.study-table-title {
    text-align: left;
    padding-left: 4px;
    padding-right: 4px;
    vertical-align: middle;
    line-height: 1.2rem;
    position: sticky;
}

/* .study-table> :not(:first-child) {
  border-top: 0px !important;
} */

.study-table {
    table-layout: fixed;
}

.study-table> :nth-child(odd)>tr>td {
    background-color: var(--study-odd-bg-color);
}

.study-table> :nth-child(even)>tr>td {
    background-color: var(--study-even-bg-color);
}

/* only on the first child of each tbody to prevent hover over the labels row */
.study-table>tbody>tr:first-child:hover>* {
    background-color: var(--study-hover-color);
}

.study-table>tbody>tr.study-row-expanded:hover>* {
    background-color: var(--study-details-bg-color);
}

.study-table>tbody>tr.study-details-expanded:hover>* {
    background-color: var(--study-details-bg-color);
}

.study-table> :last-child {
    border-bottom-width: thin;
}

.study-table tr:hover {
    background-color: var(--study-hover-color);
}


.study-table-filters {
    background-color: var(--study-table-filter-bg-color);
}

.study-table-filters>th {
    background-color: var(--study-table-filter-bg-color);
}

.study-table-filters>th>button {
    background-color: var(--bs-table-bg);
}

.study-table-filters>th {
    text-align: left;
    padding-left: 6px !important;
    padding-top: 0px;
    padding-bottom: 0px;
    margin-bottom: 5px;
    vertical-align: middle;
}

.study-table td {
    text-align: left;
    padding-left: 10px;
}

.study-list-alert {
    margin-top: var(--filter-margin);
    margin-bottom: var(--filter-margin);
    padding-top: var(--filter-padding);
    padding-bottom: var(--filter-padding);
}

.study-list-bulk-buttons {
    margin-top: var(--filter-margin);
}

.is-invalid-filter {
    /* background-color: #f7dddf !important; */
    border-color: red !important;
    box-shadow: 0 0 0 .25rem rgba(255, 0, 0, .25) !important;
}

.alert-icon {
    margin-right: 0.7rem;
}

.study-table-actions>th {
    background-color: var(--study-table-actions-bg-color) !important;
    vertical-align: middle;
}

.study-table-actions>th>div {
    background-color: var(--study-table-actions-bg-color) !important;
    text-align: left;
}

.study-details-table {
    margin-top: var(--details-top-margin);
    margin-left: 5%;
    width: 95% !important;
    font-size: 0.9rem;
    table-layout: fixed;
}

.study-details-table>:not(caption)>*>* {
    background-color: var(--study-details-bg-color) !important;
}

.study-details-table>*>* {
    background-color: var(--study-details-bg-color) !important;
}

.study-details-table td {
    vertical-align: top;
}

.remote-browsing-warning {
    background-color: var(--study-list-remote-bg-color);
    text-align: center;
    font-weight: 500;
    height: 2rem;
    line-height: 2rem;
}

.title-container {
    /* position: relative; */
    width: 100%;
    height: 2rem;
}

.title-text {
    position: absolute;
    top: 0;
    left: 0;
    padding-left: 2px;
    padding-right: 2px;
    width: 100%;
    height: 100%;
    border-left: 1px;
    border-right: 0px;
    border-top: 0px;
    border-bottom: 0px;
    border-style: solid;
    border-color: var(--study-table-actions-bg-color);
    text-overflow: ellipsis;
    overflow: hidden;
}

.is-orderable {
    cursor: pointer;
    user-select: none;
}

.title-arrow {
    position: absolute;
    font-size: medium;
    bottom: 0;
    right: 0;
    padding-right: 5px;
}

/* ───── Mobile portrait & landscape breakpoint ───── */
@include mobile-any {
    .col-prefix {
        width: 36px !important;
        min-width: 36px !important;
        max-width: 36px !important;
        padding: 0 4px !important;
    }

    /* Ensure DatePicker in table cell is visible and clickable */
    :deep(.dp__main),
    :deep(.dp__input) {
        width: 100% !important;
        min-width: 100px !important;
        height: 36px !important;
        cursor: pointer !important;
    }
}
</style>

<style scoped>
.mobile-study-list {
    width: 100vw;
    overflow-x: hidden;
}

.mobile-filter-strip {
    display: flex;
    flex-direction: column;
    padding: 6px 8px;
    background-color: var(--study-table-filter-bg-color);
    border-bottom: 2px solid var(--bs-border-color);
}

/* Landscape: force a single-row filter strip */
.mobile-landscape .mobile-filter-column {
    flex: 1 1 0;
    min-width: 0;
}

.mobile-filter-row {
    display: flex;
    flex-wrap: wrap;
    gap: 8px;
    width: 100%;
}

/* Each filter column wraps onto a new line when it cannot keep its target width */
.mobile-filter-column {
    display: flex;
    flex-direction: column;
    flex: 1 1 130px;
    min-width: 0;
}

/* Clear & Search buttons: kept together, always on the same line */
.mobile-filter-buttons {
    display: flex;
    flex-wrap: nowrap;
    gap: 8px;
}

.mobile-clear-col {
    flex: 0 0 44px;
    min-width: 44px;
    max-width: 44px;
}

.mobile-search-col {
    flex: 1 1 90px;
    min-width: 90px;
}

.mobile-filter-title {
    position: relative;
    font-size: 0.85rem;
    font-weight: 600;
    color: var(--bs-body-color);
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
    display: flex;
    align-items: center;
    height: 28px;
    line-height: 28px;
    margin: 0;
    padding: 0;
    padding-right: 18px; /* reserve space for the order arrow */
}

.mobile-filter-input {
    height: 36px;
    width: 100%;
    display: flex;
    align-items: center;
}

.mobile-filter-title.is-orderable {
    cursor: pointer;
    text-decoration: underline dotted;
}

.title-arrow {
    margin-left: 4px;
}

.mobile-action-bar {
    border-bottom: 1px solid var(--bs-border-color);
    box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
}

.mobile-labels-filter {
    background-color: var(--study-odd-bg-color, #2b3035);
    border: 1px solid var(--bs-border-color);
}


.mobile-clear-col .btn,
.mobile-search-col .btn {
    height: 36px;
}

.mobile-filter-column :deep(.form-control:not(.search-button)),
.mobile-filter-column :deep(.dp__input),
.mobile-filter-column .btn.dropdown-toggle {
    height: 36px !important;
    display: flex;
    align-items: center;
}

.mobile-filter-column :deep(.dp__input) {
    padding-top: 0 !important;
    padding-bottom: 0 !important;
}

.mobile-filter-column :deep(.form-control:not(.search-button)) {
    justify-content: flex-start;
    padding-left: 8px;
}

</style>