<script>
import axios from "axios"
import StudyItem from "./StudyItem.vue"
import api from "../orthancApi"
import { mapState } from "vuex"
import { baseOe2Url } from "../globalConfigurations"

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
            clearingFilter: false,
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
        },
    },
    watch: {
        '$route.params.filters': async function (filters) { // the watch is used when, e.g, clicking on an upload report link
            this.updateFilterFromRoute(filters);
        },
        '$route': async function () { // the watch is used when, e.g, clicking on an upload report link
            console.log("route changed")
        },
        isConfigurationLoaded(newValue, oldValue) {
            // this is used when opening the page directly from a url with filters
            console.log("Configuration has been loaded, updating study filter: ",this.$route.params.filters);
            this.updateFilterFromRoute(this.$route.params.filters);
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
            console.log("filterPatientID 0", newValue);
            if (!this.clearingFilter) {
                console.log("filterPatientID 1", newValue);
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
        console.log("mounted");
        if (this.isConfigurationLoaded) {
            this.updateFilterFromRoute(this.$route.params.filters);
        }
        
    },
    methods: {
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
        },
        async clearFilters() {
            this.clearingFilter = true;

            this.emptyFilterForm();

            await this.$store.dispatch('studies/clearFilter');

            this.updateUrl();

            this.clearingFilter = false;
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

            let newUrl = baseOe2Url;
            if (activeFilters.length > 0) {
                newUrl = baseOe2Url + "/filtered-studies/" + activeFilters.join('/');
            }
            history.pushState({}, '', newUrl);
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
                <th
                    v-for="columnTag in uiOptions.StudyListColumns"
                    :key="columnTag"
                    data-bs-toggle="tooltip"
                    v-bind:title="columns[columnTag].tooltip"
                    v-bind:width="columns[columnTag].width"
                    v-bind:class="'study-table-header cut-text ' + columns[columnTag].extraClasses"
                >{{ columns[columnTag].title }}</th>
            </thead>
            <thead class="study-filter">
                <th scope="col" class="px-2">
                    <button
                        @click="clearFilters"
                        type="button"
                        class="form-control study-list-filter btn btn-shadow-none"
                        data-bs-toggle="tooltip"
                        title="Clear filter"
                    >
                        <i class="far fa-times-circle"></i>
                    </button>
                </th>
                <th
                    v-for="columnTag in uiOptions.StudyListColumns"
                    :key="columnTag"
                >
                    <input
                        v-if="columnTag=='StudyDate'"
                        type="text"
                        class="form-control study-list-filter"
                        v-model="filterStudyDate"
                        placeholder="20220130"
                    />
                    <input
                        v-if="columnTag=='AccessionNumber'"
                        type="text"
                        class="form-control study-list-filter"
                        v-model="filterAccessionNumber"
                        placeholder="1234"
                    />
                    <input
                        v-if="columnTag=='PatientID'"
                        type="text"
                        class="form-control study-list-filter"
                        v-model="filterPatientID"
                        placeholder="1234"
                    />
                    <input
                        v-if="columnTag=='PatientName'"
                        type="text"
                        class="form-control study-list-filter"
                        v-model="filterPatientName"
                        placeholder="John^Doe"
                    />                    
                    <input
                        v-if="columnTag=='PatientBirthDate'"
                        type="text"
                        class="form-control study-list-filter"
                        v-model="filterPatientBirthDate"
                        placeholder="19740815"
                    />      
                    <input
                        v-if="columnTag=='StudyDescription'"
                        type="text"
                        class="form-control study-list-filter"
                        v-model="filterStudyDescription"
                        placeholder="Chest"
                    />                                  
                </th>
            </thead>
            <StudyItem
                v-for="studyId in studiesIds"
                :key="studyId"
                :studyId="studyId"
                @deletedStudy="onDeletedStudy"
            ></StudyItem>
        </table>
        <div v-if="notShowingAllResults" class="alert alert-danger bottom-fixed-alert" role="alert">
            <i class="bi bi-exclamation-triangle-fill"></i> Not showing all results. You should refine your search criteria !
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

.study-table > :not(:first-child) {
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