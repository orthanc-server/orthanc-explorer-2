<script>
import SeriesItem from "./SeriesItem.vue"
import SeriesList from "./SeriesList.vue";
import { mapState, mapGetters } from "vuex"
import ResourceButtonGroup from "./ResourceButtonGroup.vue";
import ResourceDetailText from "./ResourceDetailText.vue";
import api from "../orthancApi";
import LabelsEditor from "./LabelsEditor.vue";

export default {
    props: ['studyId', 'studyMainDicomTags', 'patientMainDicomTags', 'labels'],
    emits: ["deletedStudy", "studyLabelsUpdated"],
    setup() {
    },
    data() {
        return {
            samePatientStudiesCount: 0,
            studySeries: [],
            hasLoadedSamePatientsStudiesCount: false,
            allLabelsLocalCopy: new Set()
        };
    },
    async created() {
        this.allLabelsLocalCopy = await api.loadAllLabels();
        this.messageBus.on('added-series-to-study-' + this.studyId, this.reloadSeriesList);
    },
    async mounted() {
        this.samePatientStudiesCount = (await api.getSamePatientStudies(this.patientMainDicomTags, this.uiOptions.ShowSamePatientStudiesFilter)).length;
        this.reloadSeriesList();
        this.hasLoadedSamePatientsStudiesCount = true;
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
        }),
        samePatientStudiesLink() {
            let filters = [];
            for (let tag of this.uiOptions.ShowSamePatientStudiesFilter) {
                if (tag in this.patientMainDicomTags) {
                    if (["PatientBirthDate"].indexOf(tag) >= 0) {
                        filters.push(tag + "=" + this.patientMainDicomTags[tag] + "");
                    } else {
                        filters.push(tag + "=\"" + this.patientMainDicomTags[tag] + "\"");
                    }
                }
            }
            return "/filtered-studies?" + filters.join('&');
        }
        
    },
    components: { SeriesItem, SeriesList, ResourceButtonGroup, ResourceDetailText, LabelsEditor },
    methods: {
        onDeletedStudy() {
            this.$emit("deletedStudy", this.studyId);
        },
        async reloadSeriesList() {
            this.studySeries = (await api.getStudySeries(this.studyId));
        },
        async labelsUpdated() {
            this.$emit("studyLabelsUpdated", this.studyId);

            // update the side bar
            // setTimeout(() => {this.$store.dispatch('labels/refresh')}, 100);
        }
    },
    watch: {
    }

}
</script>


<template>
    <table class="table table-responsive table-sm study-details-table">
        <tr v-if="uiOptions.EnableEditLabels">
            <td colspan="100%">
                <LabelsEditor :labels="labels" :studyId="studyId" @labelsUpdated="labelsUpdated"></LabelsEditor>
            </td>
        </tr>
        <tr v-if="!uiOptions.EnableEditLabels">
            <td colspan="100%">
                {{  $t('labels.study_details_title') }}
                <span v-for="label in labelsModel" :key="label" class="label badge">{{ label }}</span>
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
                    <router-link :to='samePatientStudiesLink' >
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
                    :resourceDicomUid="this.studyMainDicomTags.StudyInstanceUID" :studySeries="this.studySeries" @deletedResource="onDeletedStudy">
                </ResourceButtonGroup>
            </td>
        </tr>
        <tr>
            <td colspan="100">
                <SeriesList :studyId="this.studyId" :studyMainDicomTags="this.studyMainDicomTags"
                    :patientMainDicomTags="this.patientMainDicomTags" :studySeries="this.studySeries" @deletedStudy="onDeletedStudy"></SeriesList>
            </td>
        </tr>
    </table>
</template>

<style>
.study-button-group i {
    font-size: 1.4rem;
}

.invalid-label-tips {
    color: red;
    font-weight: 600;
    margin-left: 2rem;
} 

.invalid-label-tips-hidden {
    display: none;
} 

</style>