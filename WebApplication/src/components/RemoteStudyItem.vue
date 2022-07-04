<script>
import RemoteStudyDetails from "./RemoteStudyDetails.vue";
import { mapState } from "vuex"
import bootstrap from "bootstrap/dist/js/bootstrap.bundle.min.js"

export default {
    props: ["studyFields", "id", "remoteMode", "remoteSource"],  // studyFields = the "answer" from Orthanc to /modalities/.../queries -> /queries/.../answers?expand
    emits: [],
    data() {
        return {
            loaded: false,
            expanded: false,
            collapseElement: null
        };
    },
    async mounted() {
        // const study = this.studies.filter(s => s["ID"] == this.studyId)[0];
        this.loaded = true;

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
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
            // studies: state => state.studies.studies,
        }),        
        modalitiesInStudyForDisplay() {
            return this.studyFields.ModalitiesInStudy.split('\\').join(',');
        },
    },
    components: { RemoteStudyDetails }
}
</script>


<template>
    <tbody>
        <tr
            v-if="loaded"
            :class="{ 'study-row-collapsed': !expanded, 'study-row-expanded': expanded }"
        >
            <td></td>
            <td></td>
            <td
                v-for="columnTag in uiOptions.StudyListColumns"
                :key="columnTag"
                class="cut-text"
                :class="{ 'text-center' : columnTag in ['modalities', 'seriesCount']}"
                data-bs-toggle="collapse"
                v-bind:data-bs-target="'#study-details-' + this.id"
            >
                <span
                    v-if="columnTag=='StudyDate'"
                    data-bs-toggle="tooltip"
                    v-bind:title="studyFields.StudyDate"
                >{{ studyFields.StudyDate }}
                </span>
                <span
                    v-if="columnTag=='AccessionNumber'"
                    data-bs-toggle="tooltip"
                    v-bind:title="studyFields.AccessionNumber"
                >{{ studyFields.AccessionNumber }}
                </span>
                <span
                    v-if="columnTag=='PatientID'"
                    data-bs-toggle="tooltip"
                    v-bind:title="studyFields.PatientID"
                >{{ studyFields.PatientID }}
                </span>
                <span
                    v-if="columnTag=='PatientName'"
                    data-bs-toggle="tooltip"
                    v-bind:title="studyFields.PatientName"
                >{{ studyFields.PatientName }}
                </span>
                <span
                    v-if="columnTag=='PatientBirthDate'"
                    data-bs-toggle="tooltip"
                    v-bind:title="studyFields.PatientBirthDate"
                >{{ studyFields.PatientBirthDate }}
                </span>
                <span
                    v-if="columnTag=='StudyDescription'"
                    data-bs-toggle="tooltip"
                    v-bind:title="studyFields.StudyDescription"
                >{{ studyFields.StudyDescription }}
                </span>
                <span
                    v-if="columnTag=='modalities'"
                    data-bs-toggle="tooltip"
                    v-bind:title="modalitiesInStudyForDisplay"
                >{{ modalitiesInStudyForDisplay }}
                </span>
                <span
                    v-if="columnTag=='seriesCount'"
                >{{ studyFields.NumberOfStudyRelatedSeries }}
                </span>
            </td>
        </tr>
        <tr
            v-show="loaded"
            class="collapse"
            :class="{ 'study-details-collapsed': !expanded, 'study-details-expanded': expanded }"
            v-bind:id="'study-details-' + this.id"
            ref="study-collapsible-details"
        >
            <td v-if="loaded && expanded" colspan="100">
                <RemoteStudyDetails
                    :studyFields="this.studyFields"
                    :id="this.id"
                    :remoteMode="this.remoteMode"
                    :remoteSource="this.remoteSource"
                ></RemoteStudyDetails>
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