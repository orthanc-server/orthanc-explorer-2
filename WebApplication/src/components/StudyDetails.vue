<script>
import SeriesItem from "./SeriesItem.vue"
import SeriesList from "./SeriesList.vue";
import { mapState, mapGetters } from "vuex"
import ResourceButtonGroup from "./ResourceButtonGroup.vue";
import ResourceDetailText from "./ResourceDetailText.vue";
import AuditLogs from "./AuditLogs.vue";
import api from "../orthancApi";
import LabelsEditor from "./LabelsEditor.vue";
import SourceType from '../helpers/source-type';

export default {
    props: ['studyId', 'studyMainDicomTags', 'patientMainDicomTags', 'labels'],
    emits: ["deletedStudy"],
    setup() {
    },
    data() {
        return {
            samePatientStudiesCount: 0,
            studySeries: [],
            hasLoadedSamePatientsStudiesCount: false,
            allLabelsLocalCopy: new Set(),
            studyMainDicomTagsLocalCopy: {},
            remoteStudyFoundLocally: false,
            labelsComponentKey: 0  // to force refresh of the labels editor when allLabels is modified
        };
    },
    async created() {
        this.allLabelsLocalCopy = await api.loadAllLabels();
        this.messageBus.on('added-series-to-study-' + this.studyId, this.reloadSeriesList);
    },
    async mounted() {
        this.samePatientStudiesCount = (await api.getSamePatientStudies(this.patientMainDicomTags, this.uiOptions.ShowSamePatientStudiesFilter)).length;
        this.studyMainDicomTagsLocalCopy = { ...this.studyMainDicomTags }; // make a copy to be able to modify it
        await this.reloadSeriesList();
        this.hasLoadedSamePatientsStudiesCount = true;

        if (this.studiesSourceType == SourceType.REMOTE_DICOM || this.studiesSourceType == SourceType.REMOTE_DICOM_WEB) {
            this.remoteStudyFoundLocally = (await api.studyExists(this.studyMainDicomTagsLocalCopy.StudyInstanceUID));
        }

        if (this.studiesSourceType == SourceType.LOCAL_ORTHANC) {
            // if VisitComments is too long, it is actually not stored in DB -> in this case, get it from an instance
            for (const t of this.uiOptions.StudyMainTags) {
                if (!(t in this.studyMainDicomTagsLocalCopy) && ['VisitComments'].includes(t)) {
                    console.info("Possibly missing tag from DB: " + t + ", will get it from an instance");
                    let instances = await api.getStudyInstancesIds(this.studyId);
                    let instanceTags = await api.getInstanceTags(instances[0]);
                    for (const [k, v] of Object.entries(instanceTags)) {
                        if (v['Name'] == t) {
                            this.studyMainDicomTagsLocalCopy[t] = v['Value'];
                        }
                    }
                }
            }
        }
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
            allLabels: state => state.labels.allLabels,
            studiesSourceType: state => state.studies.sourceType,
            studiesRemoteSource: state => state.studies.remoteSource,
        }),
        showLabels() {
            if (this.studiesSourceType == SourceType.LOCAL_ORTHANC) {
                return true;
            } else {
                return false;
            }
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
        },
        isLocalOrthanc() {
            return this.studiesSourceType == SourceType.LOCAL_ORTHANC;
        },
        isRemoteSource() {
            return this.studiesSourceType == SourceType.REMOTE_DICOM || this.studiesSourceType == SourceType.REMOTE_DICOM_WEB;
        },
        sameLocalStudyLink() {
            return "/filtered-studies?StudyInstanceUID=" + this.studyMainDicomTagsLocalCopy.StudyInstanceUID;
        }
    },
    watch: {
        allLabels(newValue, oldValue) {
            this.labelsComponentKey++; // force refresh
        }
    },
    components: { SeriesItem, SeriesList, ResourceButtonGroup, ResourceDetailText, LabelsEditor, AuditLogs },
    methods: {
        onDeletedStudy() {
            this.$emit("deletedStudy", this.studyId);
        },
        async reloadSeriesList() {
            if (this.studiesSourceType == SourceType.LOCAL_ORTHANC) {
                this.studySeries = (await api.getStudySeries(this.studyId));
            } else if (this.studiesSourceType == SourceType.REMOTE_DICOM) {
                let remoteSeries = (await api.remoteDicomFind("Series", this.studiesRemoteSource, {
                    "StudyInstanceUID": this.studyMainDicomTagsLocalCopy.StudyInstanceUID,
                    "PatientID": this.patientMainDicomTags.PatientID,
                    "NumberOfSeriesRelatedInstances": "",
                    "Modality": "",
                    "SeriesDescription": "",
                    "SeriesNumber": ""
                },
                    false /* isUnique */));
                this.studySeries = remoteSeries.map(s => {
                    return {
                        "ID": s["SeriesInstanceUID"],
                        "MainDicomTags": s
                    }
                })
            } else if (this.studiesSourceType == SourceType.REMOTE_DICOM_WEB) {
                let remoteSeries = (await api.qidoRs("Series", this.studiesRemoteSource, {
                    "StudyInstanceUID": this.studyMainDicomTagsLocalCopy.StudyInstanceUID,
                    "NumberOfSeriesRelatedInstances": "",
                    "Modality": "",
                    "SeriesDescription": "",
                    "SeriesNumber": "",
                    "SeriesDate": "",
                    "SeriesTime": "",
                    "BodyPartExamined": "",
                    "ProtocolName": ""
                },
                    false /* isUnique */));
                this.studySeries = remoteSeries.map(s => {
                    return {
                        "ID": s["SeriesInstanceUID"],
                        "MainDicomTags": s
                    }
                })
            }
        },
    }

}
</script>


<template>
    <table class="table table-responsive table-sm study-details-table">
        <tbody>
            <tr v-if="showLabels && uiOptions.EnableEditLabels">
                <td colspan="100%">
                    <LabelsEditor :labels="labels" :title="'labels.study_details_title'" :key="labelsComponentKey"
                        :studyId="studyId"></LabelsEditor>
                </td>
            </tr>
            <tr v-if="showLabels && !uiOptions.EnableEditLabels">
                <td colspan="100%">
                    {{ $t('labels.study_details_title') }}
                    <span v-for="label in labels" :key="label" class="label badge">{{ label }}</span>
                </td>
            </tr>
            <tr>
                <td width="40%" class="cut-text">
                    <ul>
                        <ResourceDetailText v-for="tag in uiOptions.StudyMainTags" :key="tag"
                            :tags="studyMainDicomTagsLocalCopy" :tag="tag" :showIfEmpty="true"></ResourceDetailText>
                    </ul>
                </td>
                <td width="40%" class="cut-text">
                    <ul>
                        <ResourceDetailText v-for="tag in uiOptions.PatientMainTags" :key="tag"
                            :tags="patientMainDicomTags" :tag="tag" :showIfEmpty="true"></ResourceDetailText>
                    </ul>
                    <p v-if="isLocalOrthanc && hasLoadedSamePatientsStudiesCount && samePatientStudiesCount > 1"
                        class="info-text">
                        {{ $t('this_patient_has_other_studies', { count: samePatientStudiesCount }) }}.
                        <router-link :to='samePatientStudiesLink'>
                            {{ $t('this_patient_has_other_studies_show') }}
                        </router-link>
                    </p>
                    <p v-if="isLocalOrthanc && hasLoadedSamePatientsStudiesCount && samePatientStudiesCount == 1"
                        class="info-text">
                        {{ $t('this_patient_has_no_other_studies') }}
                    </p>
                    <p v-if="isRemoteSource && hasLoadedSamePatientsStudiesCount && samePatientStudiesCount > 1"
                        class="info-text">
                        {{ $t('this_remote_patient_has_local_studies', { count: samePatientStudiesCount }) }}
                        <router-link :to='samePatientStudiesLink'>
                            {{ $t('this_patient_has_other_studies_show') }}
                        </router-link>
                    </p>
                    <p v-if="isRemoteSource && remoteStudyFoundLocally" class="info-text">
                        {{ $t('this_study_is_already_stored_locally') }}
                        <router-link :to='sameLocalStudyLink'>
                            {{ $t('this_study_is_already_stored_locally_show') }}
                        </router-link>
                    </p>

                </td>
                <td width="20%" class="study-button-group">
                    <ResourceButtonGroup :resourceOrthancId="this.studyId" :resourceLevel="'study'"
                        :patientMainDicomTags="this.patientMainDicomTags"
                        :studyMainDicomTags="this.studyMainDicomTagsLocalCopy"
                        :resourceDicomUid="this.studyMainDicomTagsLocalCopy.StudyInstanceUID"
                        :studySeries="this.studySeries" @deletedResource="onDeletedStudy">
                    </ResourceButtonGroup>
                </td>
            </tr>
            <tr v-if="uiOptions.EnableAuditLogs">
                <td colspan="100">
                    <router-link class="router-link" :to="'/audit-logs?resource-id=' + this.studyId">{{
                        $t('audit_logs.expand_logs') }}</router-link>
                </td>
            </tr>
            <tr>
                <td colspan="100">
                    <SeriesList :studyId="this.studyId" :studyMainDicomTags="this.studyMainDicomTagsLocalCopy"
                        :patientMainDicomTags="this.patientMainDicomTags" :studySeries="this.studySeries"
                        @deletedStudy="onDeletedStudy"></SeriesList>
                </td>
            </tr>
        </tbody>
    </table>
</template>

<style>
.study-button-group i {
    font-size: 1.4rem;
}

.info-text {
    text-wrap: wrap;
}
</style>