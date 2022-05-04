<script>
import SeriesItem from "./SeriesItem.vue"
import SeriesList from "./SeriesList.vue";
import ResourceButtonGroup from "./ResourceButtonGroup.vue";

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
    components: { SeriesItem, SeriesList, ResourceButtonGroup },
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
                    <li class="d-flex">
                        <span class="details-label">Study Date:</span>
                        <span class="details">{{ studyMainDicomTags.StudyDate }}</span>
                    </li>
                    <li class="d-flex">
                        <span class="details-label">Study Time:</span>
                        <span class="details">{{ studyMainDicomTags.StudyTime }}</span>
                    </li>
                    <li class="d-flex">
                        <span class="details-label">Study Description:</span>
                        <span class="details">{{ studyMainDicomTags.StudyDescription }}</span>
                    </li>
                    <li class="d-flex">
                        <span class="details-label">Accession Number:</span>
                        <span class="details">{{ studyMainDicomTags.AccessionNumber }}</span>
                    </li>
                    <li class="d-flex">
                        <span class="details-label">Study ID:</span>
                        <span class="details">{{ studyMainDicomTags.StudyID }}</span>
                    </li>
                    <li class="d-flex">
                        <span class="details-label d-inline-block text-truncate">Study Instance UID:</span>
                        <span class="details">{{ studyMainDicomTags.StudyInstanceUID }}</span>
                    </li>
                    <li class="d-flex">
                        <span class="details-label">ReferringPhysicianName:</span>
                        <span class="details">{{ studyMainDicomTags.ReferringPhysicianName }}</span>
                    </li>
                </ul>
            </td>
            <td width="40%" class="cut-text">
                <ul>
                    <li class="d-flex">
                        <span class="details-label">Patient ID:</span>
                        <span class="details">{{ patientMainDicomTags.PatientID }}</span>
                    </li>
                    <li class="d-flex">
                        <span class="details-label">Patient Name:</span>
                        <span class="details">{{ patientMainDicomTags.PatientName }}</span>
                    </li>
                    <li class="d-flex">
                        <span class="details-label">Patient Birth Date:</span>
                        <span class="details">{{ patientMainDicomTags.PatientBirthDate }}</span>
                    </li>
                    <li class="d-flex">
                        <span class="details-label">Patient Sex:</span>
                        <span class="details">{{ patientMainDicomTags.PatientSex }}</span>
                    </li>
                </ul>
            </td>
            <td width="20%" class="study-button-group">
                <ResourceButtonGroup
                :resourceOrthancId="this.studyId"
                :resourceLevel="'study'"
                :resourceDicomUid="this.studyMainDicomTags.StudyInstanceUID"
                @deletedResource="onDeletedStudy"
                ></ResourceButtonGroup>
            </td>
        </tr>
        <tr>
            <td colspan="100">
                <SeriesList
                    :studyId="this.studyId"
                    @deletedStudy="onDeletedStudy"
                ></SeriesList>
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

.details-label {
    font-weight: 700;
    max-width: 30%;
    overflow: hidden;
    text-overflow: ellipsis;
}

.details {
    margin-left: auto !important;
    font-weight: 500;
    max-width: 25vw;
    overflow: hidden;
    text-overflow: ellipsis;
}

</style>
<style>
.study-button-group i {
    font-size: 1.4rem;
}

</style>
