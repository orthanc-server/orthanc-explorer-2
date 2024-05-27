<script>
import SeriesItem from "./SeriesItem.vue"
import SeriesList from "./SeriesList.vue";
import { mapState, mapGetters } from "vuex"
import ResourceButtonGroup from "./ResourceButtonGroup.vue";
import ResourceDetailText from "./ResourceDetailText.vue";
import api from "../orthancApi";
import Tags from "bootstrap5-tags/tags.js"

window.filterLabel = (str) => {
    const regexLabel = new RegExp("^[0-9\-\_a-zA-Z]+$");
    if (!regexLabel.test(str)) {
        const invalidLabelTips = document.querySelectorAll('.invalid-label-tips');
            invalidLabelTips.forEach(element => {
                element.classList.remove('invalid-label-tips-hidden');
            })
        setTimeout(() => {
            const invalidLabelTips = document.querySelectorAll('.invalid-label-tips');
            invalidLabelTips.forEach(element => {
                element.classList.add('invalid-label-tips-hidden');
            })
        }, 8000);
    } 

    return str.replace(/[^0-9\-\_a-zA-Z]/gi, '');
}

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
            labelsModel: [],
            allLabelsLocalCopy: new Set()
        };
    },
    async created() {
        this.labelsModel = this.labels;
        this.allLabelsLocalCopy = await api.loadAllLabels();
        this.messageBus.on('added-series-to-study-' + this.studyId, this.reloadSeriesList);
    },
    async mounted() {
        this.samePatientStudiesCount = (await api.getSamePatientStudies(this.patientMainDicomTags, this.uiOptions.ShowSamePatientStudiesFilter)).length;
        this.reloadSeriesList();
        this.hasLoadedSamePatientsStudiesCount = true;
        Tags.init();
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
        }),
        selectedValues() {
            return this.labelsModel.join(",");
        },
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
    components: { SeriesItem, SeriesList, ResourceButtonGroup, ResourceDetailText },
    methods: {
        onDeletedStudy() {
            this.$emit("deletedStudy", this.studyId);
        },
        hasLabel(label) {
            return this.labelsModel.includes(label);
        },
        async reloadSeriesList() {
            this.studySeries = (await api.getStudySeries(this.studyId));
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
                <label for="labelsEdit" class="form-label">{{  $t('labels.study_details_title') }} <span class="invalid-label-tips invalid-label-tips-hidden">{{ $t('labels.valid_alphabet_warning') }}</span></label>
                <select class="form-select" id="labelsEdit" name="tags[]" v-model="labelsModel" multiple
                    data-allow-clear="true" data-show-all-suggestions="true" data-allow-new="true" data-badge-style="info"
                    data-input-filter="filterLabel"
                    :placeholder="$t('labels.add_labels_placeholder')">
                    <option v-for="label in allLabelsLocalCopy" :key="label" :value="label" :selected="hasLabel(label)">{{ label }}
                    </option>
                </select>
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