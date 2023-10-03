<script>
import SeriesItem from "./SeriesItem.vue"
import SeriesList from "./SeriesList.vue";
import { mapState, mapGetters } from "vuex"
import ResourceButtonGroup from "./ResourceButtonGroup.vue";
import ResourceDetailText from "./ResourceDetailText.vue";
import api from "../orthancApi";
import Tags from "bootstrap5-tags/tags.js"

export default {
    props: ['studyId', 'studyMainDicomTags', 'patientMainDicomTags', 'labels'],
    emits: ["deletedStudy", "studyLabelsUpdated"],
    setup() {
    },
    data() {
        return {
            samePatientStudiesCount: 0,
            hasLoadedSamePatientsStudiesCount: false,
            labelsModel: [],
            allLabelsLocalCopy: new Set()
        };
    },
    async created() {
        this.labelsModel = this.labels;
        this.allLabelsLocalCopy = await api.loadAllLabels();
    },
    async mounted() {
        this.samePatientStudiesCount = (await api.getSamePatientStudies(this.patientMainDicomTags['PatientID'])).length;
        this.hasLoadedSamePatientsStudiesCount = true;
        Tags.init();
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
        }),
        selectedValues() {
            return this.labelsModel.join(",");
        }
    },
    components: { SeriesItem, SeriesList, ResourceButtonGroup, ResourceDetailText },
    methods: {
        onDeletedStudy() {
            this.$emit("deletedStudy", this.studyId);
        },
        hasLabel(label) {
            return this.labelsModel.includes(label);
        }
    },
    watch: {
        labelsModel: {
            async handler(oldValue, newValue) {
                let changed = await api.updateLabels({
                    studyId: this.studyId,
                    labels: this.labelsModel
                });
                if (changed) {
                    this.$emit("studyLabelsUpdated", this.studyId);

                    // update the side bar
                    setTimeout(() => {this.$store.dispatch('labels/refresh')}, 100);
                }
            },
            deep: true
        }
    }

}
</script>


<template>
    <table class="table table-responsive table-sm study-details-table">
        <tr v-if="uiOptions.EnableEditLabels">
            <td colspan="100%">
                {{  $t('labels.study_details_title') }}
                <select class="form-select" id="labelsEdit" name="tags[]" v-model="labelsModel" multiple
                    data-allow-clear="true" data-show-all-suggestions="true" data-allow-new="true" data-badge-style="info"
                    :placeholder="$t('labels.add_labels_placeholder')">
                    <option v-for="label in allLabelsLocalCopy" :key="label" :value="label" :selected="hasLabel(label)">{{ label }}
                    </option>
                </select>
            </td>
        </tr>
        <tr v-if="!uiOptions.EnableEditLabels">
            <td colspan="100%">
                {{  $t('labels.study_details_title') }}
                <span v-for="label in labelsModel" :key="label" class="label badge bg-info">{{ label }}</span>
            </td>
        </tr>
        <tr>
            <td width="40%" class="cut-text">
                <ul>
                    <ResourceDetailText v-for="tag in uiOptions.StudyMainTags" :key="tag" :tags="studyMainDicomTags"
                        :tag="tag" :showIfEmpty="true"></ResourceDetailText>
                </ul>
            </td>
            <td width="40%" class="cut-text">
                <ul>
                    <ResourceDetailText v-for="tag in uiOptions.PatientMainTags" :key="tag" :tags="patientMainDicomTags"
                        :tag="tag" :showIfEmpty="true"></ResourceDetailText>
                </ul>
                <p v-if="hasLoadedSamePatientsStudiesCount && samePatientStudiesCount > 1">
                    {{ $t('this_patient_has_other_studies', { count: samePatientStudiesCount }) }}.
                    <router-link v-bind:to='"/filtered-studies?PatientID=\"" + patientMainDicomTags["PatientID"] + "\""' >
                        {{ $t('this_patient_has_other_studies_show') }}
                    </router-link>
                </p>
                <p v-if="hasLoadedSamePatientsStudiesCount && samePatientStudiesCount == 1">
                    {{ $t('this_patient_has_no_other_studies') }}
                </p>
            </td>
            <td width="20%" class="study-button-group">
                <ResourceButtonGroup :resourceOrthancId="this.studyId" :resourceLevel="'study'"
                    :patientMainDicomTags="this.patientMainDicomTags" :studyMainDicomTags="this.studyMainDicomTags"
                    :resourceDicomUid="this.studyMainDicomTags.StudyInstanceUID" @deletedResource="onDeletedStudy">
                </ResourceButtonGroup>
            </td>
        </tr>
        <tr>
            <td colspan="100">
                <SeriesList :studyId="this.studyId" :studyMainDicomTags="this.studyMainDicomTags"
                    :patientMainDicomTags="this.patientMainDicomTags" @deletedStudy="onDeletedStudy"></SeriesList>
            </td>
        </tr>
    </table>
</template>

<style scoped>
.study-details-table {
    margin-top: var(--details-top-margin);
    margin-left: 5%;
    width: 95% !important;
    background-color: var(--study-selected-color);
    font-size: 0.9rem;
}

.study-details-table td {
    vertical-align: top;
}

</style>
<style>
.study-button-group i {
    font-size: 1.4rem;
}
</style>