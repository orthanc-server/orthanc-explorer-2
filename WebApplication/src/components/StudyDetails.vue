<script>
import SeriesItem from "./SeriesItem.vue"
import SeriesList from "./SeriesList.vue";
import ResourceButtonGroup from "./ResourceButtonGroup.vue";
import ResourceDetailText from "./ResourceDetailText.vue";

export default {
    props: ['studyId', 'studyMainDicomTags', 'patientMainDicomTags'],
    setup() {
    },
    data() {
        return {
        };
    },
    computed: {
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
                    <ResourceDetailText :value="studyMainDicomTags.StudyDate">{{$t('study_date')}}</ResourceDetailText>
                    <ResourceDetailText :value="studyMainDicomTags.StudyTime">{{$t('study_time')}}</ResourceDetailText>
                    <ResourceDetailText :value="studyMainDicomTags.StudyDescription" :truncate="true">{{$t('study_description')}}</ResourceDetailText>
                    <ResourceDetailText :value="studyMainDicomTags.AccessionNumber">{{$t('accession_number')}}</ResourceDetailText>
                    <ResourceDetailText :value="studyMainDicomTags.StudyID">{{$t('study_id')}}</ResourceDetailText>
                    <ResourceDetailText :value="studyMainDicomTags.StudyInstanceUID" :truncate="true">Study Instance UID</ResourceDetailText>
                    <ResourceDetailText :value="studyMainDicomTags.RequestingPhysician">{{$t('requesting_physician')}}</ResourceDetailText>
                    <ResourceDetailText :value="studyMainDicomTags.ReferringPhysicianName">{{$t('referring_physician_name')}}</ResourceDetailText>
                    <ResourceDetailText :value="studyMainDicomTags.InstitutionName">{{$t('institution_name')}}</ResourceDetailText>
                </ul>
            </td>
            <td width="40%" class="cut-text">
                <ul>
                    <ResourceDetailText :value="patientMainDicomTags.PatientID">{{$t('patient_id')}}</ResourceDetailText>
                    <ResourceDetailText :value="patientMainDicomTags.PatientName">{{$t('patient_name')}}</ResourceDetailText>
                    <ResourceDetailText :value="patientMainDicomTags.PatientBirthDate">{{$t('patient_birth_date')}}</ResourceDetailText>
                    <ResourceDetailText :value="patientMainDicomTags.PatientSex">{{$t('patient_sex')}}</ResourceDetailText>
                </ul>
            </td>
            <td width="20%" class="study-button-group">
                <ResourceButtonGroup :resourceOrthancId="this.studyId" :resourceLevel="'study'"
                    :resourceDicomUid="this.studyMainDicomTags.StudyInstanceUID" @deletedResource="onDeletedStudy">
                </ResourceButtonGroup>
            </td>
        </tr>
        <tr>
            <td colspan="100">
                <SeriesList :studyId="this.studyId" @deletedStudy="onDeletedStudy"></SeriesList>
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