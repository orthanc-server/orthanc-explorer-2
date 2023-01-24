<script>
import SeriesList from "./SeriesList.vue"
import StudyDetails from "./StudyDetails.vue";
import { mapState } from "vuex"
import bootstrap from "bootstrap/dist/js/bootstrap.bundle.min.js"

export default {
    props: ["study", "isSearchButtonEnabled"],
    emits: ["deletedStudy"],
    data() {
        return {
            fields: {},
            loaded: false,
            expanded: false,
            collapseElement: null
        };
    },
    async mounted() {
        this.fields = this.study;
        this.loaded = true;
        this.seriesIds = this.study.Series;

        if (!this.$refs['study-collapsible-details']) {
            console.log('no refs: ', studyResponse);
        }
        this.$refs['study-collapsible-details'].addEventListener('show.bs.collapse', (e) => {
            if (e.target == e.currentTarget) {
                this.expanded = true;
            }
        });
        this.$refs['study-collapsible-details'].addEventListener('hide.bs.collapse', (e) => {
            if (e.target == e.currentTarget) {
                this.expanded = false;
            }
        });

        var el = this.$refs['study-collapsible-details'];
        this.collapseElement = new bootstrap.Collapse(el, {toggle: false});

        for (const [k, v] of Object.entries(this.$route.query)) {
            if (k === 'expand') {
                if (v == null || v === 'study' || v === 'series' || v === 'instance') {
                    this.collapseElement.show();
                }
            }
        }

    },
    methods: {
        onDeletedStudy(studyId) {
            this.$emit("deletedStudy", this.study.ID);
        }
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
            studies: state => state.studies.studies,
        }),        
        modalitiesInStudyForDisplay() {
            return this.fields.RequestedTags.ModalitiesInStudy.split('\\').join(',');
        },
    },
    components: { SeriesList, StudyDetails }
}
</script>


<template>
    <tbody>
        <tr
            v-if="loaded"
            :class="{ 'study-row-collapsed': !expanded, 'study-row-expanded': expanded }"
        >
            <td></td>
            <td v-if="isSearchButtonEnabled"></td>
            <td
                v-for="columnTag in uiOptions.StudyListColumns"
                :key="columnTag"
                class="cut-text"
                :class="{ 'text-center' : columnTag in ['modalities', 'seriesCount']}"
                data-bs-toggle="collapse"
                v-bind:data-bs-target="'#study-details-' + this.study.ID"
            >
                <span
                    v-if="columnTag=='StudyDate'"
                    data-bs-toggle="tooltip"
                    v-bind:title="fields.MainDicomTags.StudyDate"
                >{{ fields.MainDicomTags.StudyDate }}
                </span>
                <span
                    v-if="columnTag=='AccessionNumber'"
                    data-bs-toggle="tooltip"
                    v-bind:title="fields.MainDicomTags.AccessionNumber"
                >{{ fields.MainDicomTags.AccessionNumber }}
                </span>
                <span
                    v-if="columnTag=='PatientID'"
                    data-bs-toggle="tooltip"
                    v-bind:title="fields.PatientMainDicomTags.PatientID"
                >{{ fields.PatientMainDicomTags.PatientID }}
                </span>
                <span
                    v-if="columnTag=='PatientName'"
                    data-bs-toggle="tooltip"
                    v-bind:title="fields.PatientMainDicomTags.PatientName"
                >{{ fields.PatientMainDicomTags.PatientName }}
                </span>
                <span
                    v-if="columnTag=='PatientBirthDate'"
                    data-bs-toggle="tooltip"
                    v-bind:title="fields.PatientMainDicomTags.PatientBirthDate"
                >{{ fields.PatientMainDicomTags.PatientBirthDate }}
                </span>
                <span
                    v-if="columnTag=='StudyDescription'"
                    data-bs-toggle="tooltip"
                    v-bind:title="fields.MainDicomTags.StudyDescription"
                >{{ fields.MainDicomTags.StudyDescription }}
                </span>
                <span
                    v-if="columnTag=='modalities'"
                    data-bs-toggle="tooltip"
                    v-bind:title="modalitiesInStudyForDisplay"
                >{{ modalitiesInStudyForDisplay }}
                </span>
                <span
                    v-if="columnTag=='seriesCount'"
                >{{ fields.Series.length }}
                </span>
            </td>
        </tr>
        <tr
            v-show="loaded"
            class="collapse"
            :class="{ 'study-details-collapsed': !expanded, 'study-details-expanded': expanded }"
            v-bind:id="'study-details-' + this.study.ID"
            ref="study-collapsible-details"
        >
            <td v-if="loaded && expanded" colspan="100">
                <StudyDetails
                    :study="this.study"
                    :studyMainDicomTags="this.fields.MainDicomTags"
                    :patientMainDicomTags="this.fields.PatientMainDicomTags"
                    @deletedStudy="onDeletedStudy"
                ></StudyDetails>
            </td>
        </tr>
    </tbody>
</template>

<style scoped>
.study-row-collapsed {
}

.study-row-expanded {
    background-color: var(--study-selected-color);
    font-weight: 700;

    border-top: 2px !important;
    border-style: solid !important;
    border-color: black !important;
}

.study-row-expanded>:first-child {
    border-bottom: 0px !important;
}

.study-table > tbody > tr:hover {
    background-color: var(--study-hover-color);
}

.study-table > tbody > tr.study-row-expanded:hover {
    background-color: var(--study-selected-color);
}
.study-table > tbody > tr.study-details-expanded:hover {
    background-color: var(--study-selected-color);
}

.study-details-expanded {
    background-color: var(--study-selected-color);

    border-top: 0px !important;
    border-bottom: 2px !important;
    border-style: solid !important;
    border-color: black !important;
}

</style>