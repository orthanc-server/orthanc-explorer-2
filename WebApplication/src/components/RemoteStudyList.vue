<script>
import RemoteStudyItem from "./RemoteStudyItem.vue"
import { mapState, mapGetters } from "vuex"
import { baseOe2Url } from "../globalConfigurations"
import { translateDicomTag } from "../locales/i18n"
import $ from "jquery"
import api from "../orthancApi"

document._remoteStudyAllowedFilters = ["StudyDate", "StudyTime", "AccessionNumber", "PatientID", "PatientName", "PatientBirthDate", "StudyInstanceUID", "StudyID", "StudyDescription", "ModalitiesInStudy"];

document._remoteStudyMainStudyTags = ["StudyDate", "StudyTime", "StudyID", "AccessionNumber", "StudyDescription", "StudyInstanceUID", "ModalitiesInStudy", "NumberOfStudyRelatedSeries", "RequestingPhysician", "ReferringPhysicianName", "InstitutionName"];
document._remoteStudyMainPatientTags = ["PatientID", "PatientName","PatientBirthDate", "PatientSex"];

document._remoteStudyColumns = {
    "StudyDate": {
        "width": "7%"
    },
    "AccessionNumber": {
        "width": "11%"
    },
    "PatientID": {
        "width": "11%"
    },
    "PatientName": {
        "width": "15%"
    },
    "PatientBirthDate": {
        "width": "7%"
    },
    "StudyDescription": {
        "width": "25%"
    },
    "modalities": {
        "width": "6%",
        "extraClasses": "text-center"
    },
    "seriesCount": {
        "width": "4%",
        "extraClasses": "text-center"
    },
}



export default {
    props: [],
    emits: [],
    data() {
        return {
            filterStudyDate: '',
            filterAccessionNumber: '',
            filterPatientID: '',
            filterPatientName: '',
            filterPatientBirthDate: '',
            filterStudyDescription: '',
            filterModalities: {},
            allModalities: true,
            noneModalities: false,
            updatingFilterUi: false,
            updatingRoute: false,
            initializingModalityFilter: false,
            columns: document._remoteStudyColumns,
            remoteMode: null,
            remoteSource: null,
            studies: [],
            isSearching: false
        };
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
            isConfigurationLoaded: state => state.configuration.loaded
        }),
        isFilterEmpty() {
            return this.filterStudyDate.length == 0 && this.filterAccessionNumber.length == 0 && this.filterPatientID.length == 0 && this.filterPatientName.length == 0
              && this.filterPatientBirthDate.length == 0 && this.filterStudyDescription.length == 0 && this.getModalityFilter().length == 0;
        },
        isStudyListEmpty() {
            return this.studies.length == 0;
        }
    },
    watch: {
        '$route': async function () { // the watch is used when, e.g, clicking on the back button
            // console.log("StudyList: route changed")
            if (!this.updatingRoute) {
                this.updateFilterFromRoute(this.$route.query);
            }
            this.updatingRoute = false;
        },
        isConfigurationLoaded(newValue, oldValue) {
            // this is called when opening the page (with a filter or not)
            // console.log("StudyList: Configuration has been loaded, updating study filter: ", this.$route.params.filters);
            this.initModalityFilter();
            this.updateFilterFromRoute(this.$route.query);
        },
        filterModalities: {
            handler(newValue, oldValue) {
                if (!this.updatingFilterUi && !this.initializingModalityFilter) {
                //    console.log("StudyList: filterModalities watcher", newValue, oldValue);
                   this.updateFilter('ModalitiesInStudy', this.getModalityFilter(), null);
                }
            },
            deep: true
        },
        filterStudyDate(newValue, oldValue) {
            this.updateFilter('StudyDate', newValue, oldValue);
        },
        filterAccessionNumber(newValue, oldValue) {
            this.updateFilter('AccessionNumber', newValue, oldValue);
        },
        filterPatientID(newValue, oldValue) {
            this.updateFilter('PatientID', newValue, oldValue);
        },
        filterPatientName(newValue, oldValue) {
            this.updateFilter('PatientName', newValue, oldValue);
        },
        filterPatientBirthDate(newValue, oldValue) {
            this.updateFilter('PatientBirthDate', newValue, oldValue);
        },
        filterStudyDescription(newValue, oldValue) {
            this.updateFilter('StudyDescription', newValue, oldValue);
        },
    },
    async created() {
        // console.log("StudyList: created");
    },
    async mounted() {
        // console.log("StudyList: mounted");
        this.updateFilterFromRoute(this.$route.query);
    },
    methods: {
        columnTitle(tagName) {
            if (tagName == "seriesCount") {
                return this.$i18n.t('series_count_header');
            } else if (tagName == "modalities") {
                return translateDicomTag(this.$i18n.t, this.$i18n.te, "ModalitiesInStudy");
            } else {
                return translateDicomTag(this.$i18n.t, this.$i18n.te, tagName);
            }
        },
        columnTooltip(tagName) {
            return this.columnTitle(tagName);
        },
        clearModalityFilter() {
            // console.log("StudyList: clearModalityFilter", this.updatingFilterUi);
            for (const modality of this.uiOptions.ModalitiesFilter) {
                this.filterModalities[modality] = true;
            }
        },
        initModalityFilter() {
            // console.log("StudyList: initModalityFilter", this.updatingFilterUi);
            this.initializingModalityFilter=true;
            this.filterModalities = {};
            for (const modality of this.uiOptions.ModalitiesFilter) {
                this.filterModalities[modality] = true;
            }
            this.initializingModalityFilter=false;
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
                return "NONE"; // something that will not match !
            } else {
                this.allModalities = false;
                this.noneModalities = false;
                return selected.join('\\');
            }
        },
        updateFilter(dicomTagName, newValue, oldValue) {
            if (this.updatingFilterUi) {
                return;
            }

            // console.log("StudyList: updateFilter", this.updatingFilterUi);

            if (oldValue == null) { // not text: e.g. modalities in study -> update directly
                this._updateFilter(dicomTagName, newValue);
                return;
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
                return 2;
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
            if (value.length > 0 && !this.isFilterLongEnough(dicomTagName, value)) {
                return "is-invalid-filter";
            }
            return "";
        },
        getFilterValue(dicomTagName) {
            if (dicomTagName == "StudyDate") {
                return this.filterStudyDate;
            } else if (dicomTagName == "AccessionNumber") {
                return this.filterAccessionNumber;
            } else if (dicomTagName == "PatientID") {
                return this.filterPatientID;
            } else if (dicomTagName == "PatientName") {
                return this.filterPatientName;
            } else if (dicomTagName == "PatientBirthDate") {
                return this.filterPatientBirthDate;
            } else if (dicomTagName == "StudyDescription") {
                return this.filterStudyDescription;
            } else if (dicomTagName == "ModalitiesInStudy") {
                console.error("getFilterValue ModalitiesInStudy");
            }
        },
        _updateFilter(dicomTagName, value) {
            this.updateUrl();
        },
        async updateFilterFromRoute(filters) {
            // console.log("StudyList: updateFilterFromRoute", this.updatingFilterUi, filters);

            this.remoteMode = this.$route.query.remoteMode;
            this.remoteSource = this.$route.query.remoteSource;


            this.updatingFilterUi = true;
            if (filters === undefined) {
                await this.clearFiltersUi();
                console.warn("updateFilterFromRoute ... I think this code is not used anymore ...");  // filters=={} when default url
            } else {

                // await this.$store.dispatch('studies/clearFilterNoReload');
                var keyValueFilters = {};

                for (const [filterKey, filterValue] of Object.entries(filters)) {
                    if (document._remoteStudyAllowedFilters.indexOf(filterKey) == -1) {
                        console.log("StudyList: Not a filter Key: ", filterKey, filterValue)
                    } else {
                        keyValueFilters[filterKey] = filterValue;

                        await this.$store.dispatch('studies/updateFilterNoReload', { dicomTagName: filterKey, value: filterValue });
                    }
                }

                await this.updateFilterForm(keyValueFilters);
            }
            this.updatingFilterUi = false;
        },
        updateFilterForm(filters) {
            // console.log("StudyList: updateFilterForm", this.updatingFilterUi);
            this.emptyFilterForm();

            for (const [key, value] of Object.entries(filters)) {
                if (key == "StudyDate") {
                    this.filterStudyDate = value;
                } else if (key == "AccessionNumber") {
                    this.filterAccessionNumber = value;
                } else if (key == "PatientID") {
                    this.filterPatientID = value;
                } else if (key == "PatientName") {
                    this.filterPatientName = value;
                } else if (key == "PatientBirthDate") {
                    this.filterPatientBirthDate = value;
                } else if (key == "StudyDescription") {
                    this.filterStudyDescription = value;
                } else if (key == "ModalitiesInStudy") {
                    const modalities = value.split('\\');
                    if (modalities.length > 0) {
                        let allModalitiesInFilter = true;
                        let noneModalitiesInFilter = true;
                        for (const modality of this.uiOptions.ModalitiesFilter) {
                            const isInFilter = modalities.indexOf(modality) != -1
                            this.filterModalities[modality] = isInFilter;
                            allModalitiesInFilter &= isInFilter;
                            noneModalitiesInFilter &= !isInFilter;
                        }
                        this.allModalities = allModalitiesInFilter;
                        this.noneModalities = noneModalitiesInFilter;
                    }
                }
            }
        },
        emptyFilterForm() {
            // console.log("StudyList: emptyFilterForm", this.updatingFilterUi);
            this.filterStudyDate = '';
            this.filterAccessionNumber = '';
            this.filterPatientID = '';
            this.filterPatientName = '';
            this.filterPatientBirthDate = '';
            this.filterStudyDescription = '';
            this.clearModalityFilter();
        },
        async search() {
            if (this.isSearching) {
                api.cancelRemoteDicomFindStudies();
                this.isSearching = false;
            }
            if (this.remoteMode == 'dicom') {
                let query = {}; // list all fields that we want to retrieve (for the RemoteStudyList and the RemoteStudyDetails)
                for (let tag of document._remoteStudyMainStudyTags.concat(document._remoteStudyMainPatientTags)) {
                    query[tag] = ""
                }

                if (this.filterStudyDate.length >= 8) {
                    query["StudyDate"] = this.filterStudyDate;
                }
                if (this.filterAccessionNumber.length >= 1) {
                    query["AccessionNumber"] = "*" + this.filterAccessionNumber + "*";
                }
                if (this.filterPatientID.length >= 1) {
                    query["PatientID"] = "*" + this.filterPatientID + "*";
                }
                if (this.filterPatientName.length >= 1) {
                    query["PatientName"] = "*" + this.filterPatientName + "*";
                }
                if (this.filterPatientBirthDate.length >= 8) {
                    query["PatientBirthDate"] = this.filterPatientBirthDate;
                }
                if (this.filterStudyDescription.length >= 1) {
                    query["StudyDescription"] = "*" + this.filterStudyDescription + "*";
                }
                // if (this.filterStudyInstanceUID.length >= 1) {
                //     query["StudyInstanceUID"] = this.filterStudyInstanceUID;
                // }
                if (this.getModalityFilter().length >= 1) {
                    query["ModalitiesInStudy"] = this.getModalityFilter();
                }

                let findRequest = api.remoteDicomFindStudies(this.remoteSource, query)
                this.isSearching = true;

                findRequest.then((studies) => {
                    // console.log("received studies", studies);
                    this.studies = studies;
                    this.isSearching = false;
                })
            }
            
            this.updateUrl();
        },
        async clearFilters() {
            // console.log("StudyList: clearFilters", this.updatingFilterUi);
            await this.clearFiltersUi();
            await this.$store.dispatch('studies/clearFilter');
        },
        async clearFiltersUi() {
            // console.log("StudyList: clearFiltersUi IN");
            this.updatingFilterUi = true;

            this.emptyFilterForm();
            this.updateUrl();

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
        },
        modalityFilterClicked(ev) {  // prevent closing the drop-down at every click
            ev.stopPropagation();
        },
        closeModalityFilter(ev) {
            // simulate a click on the dropdown toggle (TODO: fix error in console)
            $("#dropdown-modalities-button").click();
            ev.preventDefault();
            ev.stopPropagation();
        },
        updateUrl() {
            let activeFilters = [];
            if (this.clipFilter("StudyDate", this.filterStudyDate)) {
                activeFilters.push('StudyDate=' + this.filterStudyDate);
            }
            if (this.clipFilter("AccessionNumber", this.filterAccessionNumber)) {
                activeFilters.push('AccessionNumber=' + this.filterAccessionNumber);
            }
            if (this.clipFilter("PatientID", this.filterPatientID)) {
                activeFilters.push('PatientID=' + this.filterPatientID);
            }
            if (this.clipFilter("PatientName", this.filterPatientName)) {
                activeFilters.push('PatientName=' + this.filterPatientName);
            }
            if (this.clipFilter("PatientBirthDate", this.filterPatientBirthDate)) {
                activeFilters.push('PatientBirthDate=' + this.filterPatientBirthDate);
            }
            if (this.clipFilter("StudyDescription", this.filterStudyDescription)) {
                activeFilters.push('StudyDescription=' + this.filterStudyDescription);
            }
            if (this.getModalityFilter()) {
                activeFilters.push('ModalitiesInStudy=' + this.getModalityFilter());
            }

            activeFilters.push('remoteMode=' + this.remoteMode);
            activeFilters.push('remoteSource=' + this.remoteSource);

            let newUrl = "";
            if (activeFilters.length > 0) {
                newUrl = "/filtered-remote-studies?" + activeFilters.join('&');
            }

            this.updatingRoute = true;  // cleared in watcher
            this.$router.replace(newUrl);
        }
    },
    components: { RemoteStudyItem }
}
</script>


<template>
    <div>
        <table class="table table-responsive table-sm study-table">
            <thead>
                <th width="2%" scope="col" class="study-table-header"></th>
                <th width="5%" scope="col" class="study-table-header"></th>
                <th v-for="columnTag in uiOptions.StudyListColumns" :key="columnTag" data-bs-toggle="tooltip"
                    v-bind:title="columnTooltip(columnTag)" v-bind:width="columns[columnTag].width"
                    v-bind:class="'study-table-header cut-text ' + columns[columnTag].extraClasses">{{
                    columnTitle(columnTag)
                    }}</th>
            </thead>
            <thead class="study-filter" v-on:keyup.enter="search">
                <th scope="col" class="px-2">
                    <button @click="clearFilters" type="button"
                        class="form-control study-list-filter btn filter-button" data-bs-toggle="tooltip"
                        title="Clear filter">
                        <i class="fa-regular fa-circle-xmark"></i>
                    </button>
                </th>
                <th scope="col" class="search-button">
                    <button @click="search" type="submit"
                        class="form-control study-list-filter btn filter-button btn-secondary" data-bs-toggle="tooltip"
                        :class="{ 'is-searching': isSearching, 'is-not-searching': !isSearching }"
                        title="Search">
                        <i v-if="!isSearching" class="fa-solid fa-magnifying-glass"></i>
                        <span v-if="isSearching" class="spinner-border spinner-border-sm" role="status" aria-hidden="true"></span>
                    </button>
                </th>
                <th v-for="columnTag in uiOptions.StudyListColumns" :key="columnTag">
                    <input v-if="columnTag == 'StudyDate'" type="text" class="form-control study-list-filter"
                        v-model="filterStudyDate" placeholder="20220130" v-bind:class="getFilterClass('StudyDate')"/>
                    <input v-if="columnTag == 'AccessionNumber'" type="text" class="form-control study-list-filter"
                        v-model="filterAccessionNumber" placeholder="1234" v-bind:class="getFilterClass('AccessionNumber')"/>
                    <input v-if="columnTag == 'PatientID'" type="text" class="form-control study-list-filter"
                        v-model="filterPatientID" placeholder="1234" v-bind:class="getFilterClass('PatientID')"/>
                    <input v-if="columnTag == 'PatientName'" type="text" class="form-control study-list-filter"
                        v-model="filterPatientName" placeholder="John^Doe" v-bind:class="getFilterClass('PatientName')"/>
                    <input v-if="columnTag == 'PatientBirthDate'" type="text" class="form-control study-list-filter"
                        v-model="filterPatientBirthDate" placeholder="19740815" v-bind:class="getFilterClass('PatientBirthDate')"/>
                    <div v-if="columnTag == 'modalities'" class="dropdown">
                        <button type="button" class="btn btn-default btn-sm filter-button dropdown-toggle" data-bs-toggle="dropdown"
                            id="dropdown-modalities-button" aria-expanded="false"><span
                                class="fa fa-list"></span>&nbsp;<span class="caret"></span></button>
                        <ul class="dropdown-menu" aria-labelledby="dropdown-modalities-button" @click="modalityFilterClicked" id="modality-filter-dropdown">
                            <li><label class="dropdown-item"><input type="checkbox" data-value="all" @click="toggleModalityFilter" v-model="allModalities" />&nbsp;{{$t('all_modalities')}}</label></li>
                            <li><label class="dropdown-item"><input type="checkbox" data-value="none" @click="toggleModalityFilter" v-model="noneModalities" />&nbsp;{{$t('no_modalities')}}</label></li>
                            <li>
                                <hr class="dropdown-divider">
                            </li>
                            <li v-for="modality in uiOptions.ModalitiesFilter" :key="modality">
                                <label class="dropdown-item"><input type="checkbox" v-bind:data-value="modality" v-model="filterModalities[modality]" />&nbsp;{{modality}}</label>
                            </li>
                            <li><button class="btn btn-primary mx-5" @click="closeModalityFilter" data-bs-toggle="dropdown">{{$t('close')}}</button></li>
                        </ul>
                    </div>
                    <input v-if="columnTag == 'StudyDescription'" type="text" class="form-control study-list-filter"
                        v-model="filterStudyDescription" placeholder="Chest" />
                </th>
            </thead>
            <RemoteStudyItem v-for="(study, index) in studies" :key="study.StudyInstanceUID" :studyFields="study" :id="index" :remoteMode="this.remoteMode" :remoteSource="this.remoteSource">
            </RemoteStudyItem>
        </table>
        <!-- <div v-if="!isSearching && notShowingAllResults" class="alert alert-danger bottom-fixed-alert" role="alert">
            <i class="bi bi-exclamation-triangle-fill"></i> {{$t('not_showing_all_results')}} !
        </div> -->
        <div v-if="!isSearching && isFilterEmpty" class="alert alert-warning bottom-fixed-alert" role="alert">
            <i class="bi bi-exclamation-triangle-fill"></i> {{$t('enter_search')}}
        </div>
        <div v-else-if="!isSearching && isStudyListEmpty" class="alert alert-warning bottom-fixed-alert" role="alert">
            <i class="bi bi-exclamation-triangle-fill"></i> {{$t('no_result_found')}}
        </div>
        <div v-else-if="isSearching" class="alert alert-secondary bottom-fixed-alert" role="alert">
             <span v-if="isSearching" class="spinner-border spinner-border-sm" role="status" aria-hidden="true"></span> {{$t('searching')}} !
        </div>
    </div>
</template>

<style>
:root {
    --filter-margin: 5px;
    --filter-padding: 2px;
}

input.form-control.study-list-filter {
    color: black;
    margin-top: var(--filter-margin);
    margin-bottom: var(--filter-margin);
    padding-top: var(--filter-padding);
    padding-bottom: var(--filter-padding);
}

.filter-button {
    border: 1px solid #ced4da;
}

.search-button {
    padding-left: 0px !important;
}

.is-not-searching {
    background-color: #0d6dfd86 !important;
    border-color: #0d6dfd86 !important;
}

.is-searching {
    background-color: #fda90d86 !important;
    border-color: #fda90d86 !important;
}

input.form-control.study-list-filter:not(:placeholder-shown) {
    background-color: white;
}

input.form-control.study-list-filter::placeholder {
    color: rgb(200, 200, 200);
}

button.form-control.study-list-filter {
    color: black;
    margin-top: var(--filter-margin);
    margin-bottom: var(--filter-margin);
    padding-top: var(--filter-padding);
    padding-bottom: var(--filter-padding);
}

.study-table-header {
    text-align: left;
    padding-left: 10px;
}

.study-table> :not(:first-child) {
    border-top: 0px !important;
}

.study-filter th {
    text-align: left;
    padding-left: 10px;
    background-color: rgb(240, 240, 240);
    padding-top: 0px;
    padding-bottom: 0px;
}

.study-table td {
    text-align: left;
    padding-left: 10px;
}

.bottom-fixed-alert {
    position: fixed !important;
    bottom: 0px;
    width: 100%;
    font-size: large;
    font-weight: 600;
    text-align: left;
    padding-left: 50px !important;
}

.is-invalid-filter {
    /* background-color: #f7dddf !important; */
    border-color: red !important;
    box-shadow: 0 0 0 .25rem rgba(255, 0, 0, .25) !important;
}

</style>