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
                    <ResourceDetailText :value="studyMainDicomTags.StudyDate">Study Date</ResourceDetailText>
                    <ResourceDetailText :value="studyMainDicomTags.StudyTime">Study Time</ResourceDetailText>
                    <ResourceDetailText :value="studyMainDicomTags.StudyDescription" :truncate="true">Study Description</ResourceDetailText>
                    <ResourceDetailText :value="studyMainDicomTags.AccessionNumber">Accession Number</ResourceDetailText>
                    <ResourceDetailText :value="studyMainDicomTags.StudyID">Study ID</ResourceDetailText>
                    <ResourceDetailText :value="studyMainDicomTags.StudyInstanceUID" :truncate="true">Study Instance UID</ResourceDetailText>
                    <ResourceDetailText :value="studyMainDicomTags.StudyDate">ReferringPhysicianName</ResourceDetailText>
                </ul>
            </td>
            <td width="40%" class="cut-text">
                <ul>
                    <ResourceDetailText :value="patientMainDicomTags.PatientID">Patient ID</ResourceDetailText>
                    <ResourceDetailText :value="patientMainDicomTags.PatientName">Patient Name</ResourceDetailText>
                    <ResourceDetailText :value="patientMainDicomTags.PatientBirthDate">Patient Birth Date</ResourceDetailText>
                    <ResourceDetailText :value="patientMainDicomTags.PatientSex">Patient Sex</ResourceDetailText>
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
