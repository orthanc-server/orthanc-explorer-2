<script>
import SeriesItem from "./SeriesItem.vue"
import SeriesList from "./SeriesList.vue";
import { mapState, mapGetters } from "vuex"
import ResourceButtonGroup from "./ResourceButtonGroup.vue";
import ResourceDetailText from "./ResourceDetailText.vue";
import api from "../orthancApi";

export default {
    props: ['studyId', 'studyMainDicomTags', 'patientMainDicomTags'],
    setup() {
    },
    data() {
        return {
            samePatientStudiesCount: 0,
            hasLoadedSamePatientsStudiesCount: false
        };
    },
    async mounted() {
        this.samePatientStudiesCount = (await api.getSamePatientStudies(this.patientMainDicomTags['PatientID'])).length;
        this.hasLoadedSamePatientsStudiesCount = true;
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
        }),
        // studyMainTags() {
        //     return this.uiOptions.StudyMainTags;
        // }
    },
    components: { SeriesItem, SeriesList, ResourceButtonGroup, ResourceDetailText },
    methods: {
        onDeletedStudy() {
            this.$emit("deletedStudy", this.studyId);
        }
    }

}
</script>


<template>
    <table class="table table-responsive table-sm study-details-table">
        <tr>
            <td width="40%" class="cut-text">
                <ul>
                    <ResourceDetailText v-for="tag in uiOptions.StudyMainTags" :key="tag" :tags="studyMainDicomTags" :tag="tag" :showIfEmpty="true"></ResourceDetailText>
                </ul>
            </td>
            <td width="40%" class="cut-text">
                <ul>
                    <ResourceDetailText v-for="tag in uiOptions.PatientMainTags" :key="tag" :tags="patientMainDicomTags" :tag="tag" :showIfEmpty="true"></ResourceDetailText>
                </ul>
                <p v-if="hasLoadedSamePatientsStudiesCount && samePatientStudiesCount > 1">
                    {{ $t('this_patient_has_other_studies', { count: samePatientStudiesCount }) }}.
                    <router-link v-bind:to="'/filtered-studies?PatientID=' + patientMainDicomTags['PatientID']">
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
                <SeriesList :studyId="this.studyId" :studyMainDicomTags="this.studyMainDicomTags" :patientMainDicomTags="this.patientMainDicomTags" @deletedStudy="onDeletedStudy"></SeriesList>
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