<script>
import StudyItem from "./StudyItem.vue"
import { mapState } from "vuex"
import { baseOe2Url } from "../globalConfigurations"
import $ from "jquery"

document._studyColumns = {
    "StudyDate": {
        "width": "7%",
        "title": "Study Date",
        "tooltip": "Study Date"
    },
    "AccessionNumber": {
        "width": "11%",
        "title": "Accession Number",
        "tooltip": "Accession Number"
    },
    "PatientID": {
        "width": "11%",
        "title": "Patient ID",
        "tooltip": "Patient ID"
    },
    "PatientName": {
        "width": "15%",
        "title": "Patient Name",
        "tooltip": "Patient Name"
    },
    "PatientBirthDate": {
        "width": "7%",
        "title": "Patient Birth Date",
        "tooltip": "Patient Birth Date"
    },
    "StudyDescription": {
        "width": "25%",
        "title": "Description",
        "tooltip": "Study Description"
    },
    "modalities": {
        "width": "6%",
        "title": "Modalities",
        "tooltip": "Modalities in Study",
        "extraClasses": "text-center"
    },
    "seriesCount": {
        "width": "4%",
        "title": "# series",
        "tooltip": "Number of Series",
        "extraClasses": "text-center"
    },
}



export default {
    props: [],
    emits: ['deletedStudy'],
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
            clearingFilter: false,
            initializingModalityFilter: false,
            columns: document._studyColumns,
        };
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
            isConfigurationLoaded: state => state.configuration.loaded,
            studiesIds: state => state.studies.studiesIds,
            statistics: state => state.studies.statistics
        }),
        notShowingAllResults() {
            if (this.studiesIds.length >= this.statistics.CountStudies) {
                return false;
            }
            return this.studiesIds.length == this.uiOptions.MaxStudiesDisplayed; // in this case, the result has been limited
        }
    },
    watch: {
        '$route.params.filters': async function (filters) { // the watch is used when, e.g, clicking on an upload report link
            this.updateFilterFromRoute(filters);
        },
        '$route': async function () { // the watch is used when, e.g, clicking on the back button
            console.log("route changed")
        },
        isConfigurationLoaded(newValue, oldValue) {
            // this is used when opening the page directly from a url with filters
            console.log("Configuration has been loaded, updating study filter: ", this.$route.params.filters);
            this.initModalityFilter();
            this.updateFilterFromRoute(this.$route.params.filters);
        },
        filterModalities: {
            handler(newValue, oldValue) {
                if (!this.clearingFilter && !this.initializingModalityFilter) {
                   console.log("modality filter changed", newValue, oldValue);
                   this.updateFilter('ModalitiesInStudy', this.getModalityFilter());
                }
            },
            deep: true
        },
        filterStudyDate(newValue, oldValue) {
            if (!this.clearingFilter) {
                this.updateFilter('StudyDate', newValue);
            }
        },
        filterAccessionNumber(newValue, oldValue) {
            if (!this.clearingFilter) {
                this.updateFilter('AccessionNumber', newValue);
            }
        },
        filterPatientID(newValue, oldValue) {
            if (!this.clearingFilter) {
                this.updateFilter('PatientID', newValue);
            }
        },
        filterPatientName(newValue, oldValue) {
            if (!this.clearingFilter) {
                this.updateFilter('PatientName', newValue);
            }
        },
        filterPatientBirthDate(newValue, oldValue) {
            if (!this.clearingFilter) {
                this.updateFilter('PatientBirthDate', newValue);
            }
        },
        filterStudyDescription(newValue, oldValue) {
            if (!this.clearingFilter) {
                this.updateFilter('StudyDescription', newValue);
            }
        },
    },
    async mounted() {
        if (this.isConfigurationLoaded) {
            this.initModalityFilter();
            this.updateFilterFromRoute(this.$route.params.filters);
        }
    },
    methods: {
        initModalityFilter() {
            console.log("init filterModalities", this.uiOptions.ModalitiesFilter);
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
        updateFilter(dicomTagName, value) {
            this.$store.dispatch('studies/updateFilter', { dicomTagName: dicomTagName, value: value });
            this.updateUrl();
        },
        async updateFilterFromRoute(filters) {
            console.log("updating filter from Route", filters);
            if (filters === undefined) {
                await this.clearFilters();
            } else {

                await this.$store.dispatch('studies/clearFilterNoReload');
                var keyValueFilters = {};
                this.clearingFilter = true;

                for (const filter of filters) {
                    const dicomTagName = filter.split('=')[0];
                    const value = filter.split('=')[1];
                    keyValueFilters[dicomTagName] = value;

                    await this.$store.dispatch('studies/updateFilterNoReload', { dicomTagName: dicomTagName, value: value });
                }


                await this.updateFilterForm(keyValueFilters);
                await this.$store.dispatch('studies/reloadFilteredStudies');
                this.clearingFilter = false;
            }
        },
        updateFilterForm(filters) {
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
                    for (const modality of this.uiOptions.ModalitiesFilter) {
                        this.filterModalities[modality] = modalities.indexOf(modality) != -1;
                    }
                }
            }
        },
        emptyFilterForm() {
            this.filterStudyDate = '';
            this.filterAccessionNumber = '';
            this.filterPatientID = '';
            this.filterPatientName = '';
            this.filterPatientBirthDate = '';
            this.filterStudyDescription = '';
            this.initModalityFilter();
        },
        async clearFilters() {
            this.clearingFilter = true;

            this.emptyFilterForm();

            await this.$store.dispatch('studies/clearFilter');

            this.updateUrl();

            this.clearingFilter = false;
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
            // console.log("click-drop-down", ev);
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
            if (this.filterStudyDate) {
                activeFilters.push('StudyDate=' + this.filterStudyDate);
            }
            if (this.filterAccessionNumber) {
                activeFilters.push('AccessionNumber=' + this.filterAccessionNumber);
            }
            if (this.filterPatientID) {
                activeFilters.push('PatientID=' + this.filterPatientID);
            }
            if (this.filterPatientName) {
                activeFilters.push('PatientName=' + this.filterPatientName);
            }
            if (this.filterPatientBirthDate) {
                activeFilters.push('PatientBirthDate=' + this.filterPatientBirthDate);
            }
            if (this.filterStudyDescription) {
                activeFilters.push('StudyDescription=' + this.filterStudyDescription);
            }
            if (this.getModalityFilter()) {
                activeFilters.push('ModalitiesInStudy=' + this.getModalityFilter());
            }

            let newUrl = baseOe2Url;
            if (activeFilters.length > 0) {
                newUrl = baseOe2Url + "/#filtered-studies/" + activeFilters.join('/');
            } else {
                newUrl = newUrl + "/#";
            }
            history.replaceState({}, '', newUrl);
        },
        onDeletedStudy(studyId) {
            this.$store.dispatch('studies/deleteStudy', { studyId: studyId });
        }
    },
    components: { StudyItem }
}
</script>


<template>
    <div>
        <table class="table table-responsive table-sm study-table">
            <thead>
                <th width="2%" scope="col" class="study-table-header"></th>
                <th v-for="columnTag in uiOptions.StudyListColumns" :key="columnTag" data-bs-toggle="tooltip"
                    v-bind:title="columns[columnTag].tooltip" v-bind:width="columns[columnTag].width"
                    v-bind:class="'study-table-header cut-text ' + columns[columnTag].extraClasses">{{
                            columns[columnTag].title
                    }}</th>
            </thead>
            <thead class="study-filter">
                <th scope="col" class="px-2">
                    <button @click="clearFilters" type="button"
                        class="form-control study-list-filter btn btn-shadow-none" data-bs-toggle="tooltip"
                        title="Clear filter">
                        <i class="far fa-times-circle"></i>
                    </button>
                </th>
                <th v-for="columnTag in uiOptions.StudyListColumns" :key="columnTag">
                    <input v-if="columnTag == 'StudyDate'" type="text" class="form-control study-list-filter"
                        v-model="filterStudyDate" placeholder="20220130" />
                    <input v-if="columnTag == 'AccessionNumber'" type="text" class="form-control study-list-filter"
                        v-model="filterAccessionNumber" placeholder="1234" />
                    <input v-if="columnTag == 'PatientID'" type="text" class="form-control study-list-filter"
                        v-model="filterPatientID" placeholder="1234" />
                    <input v-if="columnTag == 'PatientName'" type="text" class="form-control study-list-filter"
                        v-model="filterPatientName" placeholder="John^Doe" />
                    <input v-if="columnTag == 'PatientBirthDate'" type="text" class="form-control study-list-filter"
                        v-model="filterPatientBirthDate" placeholder="19740815" />
                    <div v-if="columnTag == 'modalities'" class="dropdown">
                        <button type="button" class="btn btn-default btn-sm dropdown-toggle" data-bs-toggle="dropdown"
                            id="dropdown-modalities-button" aria-expanded="false"><span
                                class="fa fa-list"></span>&nbsp;<span class="caret"></span></button>
                        <ul class="dropdown-menu" aria-labelledby="dropdown-modalities-button" @click="modalityFilterClicked" id="modality-filter-dropdown">
                            <li><label class="dropdown-item"><input type="checkbox" data-value="all" @click="toggleModalityFilter" v-model="allModalities" />&nbsp;All</label></li>
                            <li><label class="dropdown-item"><input type="checkbox" data-value="none" @click="toggleModalityFilter" v-model="noneModalities" />&nbsp;None</label></li>
                            <li>
                                <hr class="dropdown-divider">
                            </li>
                            <li v-for="modality in uiOptions.ModalitiesFilter" :key="modality">
                                <label class="dropdown-item"><input type="checkbox" v-bind:data-value="modality" v-model="filterModalities[modality]" />&nbsp;{{modality}}</label>
                            </li>
                            <li><button class="btn btn-primary mx-5" @click="closeModalityFilter" data-bs-toggle="dropdown">Close</button></li>
                        </ul>
                    </div>
                    <input v-if="columnTag == 'StudyDescription'" type="text" class="form-control study-list-filter"
                        v-model="filterStudyDescription" placeholder="Chest" />
                </th>
            </thead>
            <StudyItem v-for="studyId in studiesIds" :key="studyId" :studyId="studyId" @deletedStudy="onDeletedStudy">
            </StudyItem>
        </table>
        <div v-if="notShowingAllResults" class="alert alert-danger bottom-fixed-alert" role="alert">
            <i class="bi bi-exclamation-triangle-fill"></i> Not showing all results. You should refine your search
            criteria !
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
</style>