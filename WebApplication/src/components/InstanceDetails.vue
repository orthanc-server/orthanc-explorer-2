<script>
import api from "../orthancApi";
import ResourceButtonGroup from "./ResourceButtonGroup.vue";
import TagsTree from "./TagsTree.vue";
import { mapState, mapGetters } from "vuex"
import SourceType from '../helpers/source-type';

export default {
    props: ['instanceId', 'seriesMainDicomTags', 'studyMainDicomTags', 'patientMainDicomTags'],
    emits: ['deletedInstance'],
    data() {
        return {
            tags: {},
            headers: {},
            loaded: false
        }
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
            studiesSourceType: state => state.studies.sourceType,
        }),
    },
    async mounted() {
        if (this.studiesSourceType == SourceType.LOCAL_ORTHANC) {
            this.tags = (await api.getInstanceTags(this.instanceId));
            this.headers = (await api.getInstanceHeader(this.instanceId));
        } else if (this.studiesSourceType == SourceType.REMOTE_DICOM) {
            this.tags = {
                "0008,0018" : {
                    "Name": "SOPInstanceUID",
                    "Type": "String",
                    "Value": this.instanceId
                }
            }
        }
        this.loaded = true;
    },
    components: { ResourceButtonGroup, TagsTree },
    methods: {
        onDeletedInstance() {
            this.$emit('deletedInstance', this.instanceId)
        }
    }

}
</script>


<template>
    <table v-if="loaded" class="table table-responsive table-sm instance-details-table">
        <tr>
            <td width="80%" class="cut-text">
                <TagsTree 
                    :tags="headers">
                </TagsTree>
            </td>
            <td width="20%" class="instance-button-group">
                <ResourceButtonGroup
                :resourceOrthancId="this.instanceId"
                :resourceLevel="'instance'"
                :studyMainDicomTags="this.studyMainDicomTags"
                :seriesMainDicomTags="this.seriesMainDicomTags"
                :patientMainDicomTags="this.patientMainDicomTags"
                :instanceTags="this.tags"
                :instanceHeaders="this.headers"
                @deletedResource="onDeletedInstance"
                ></ResourceButtonGroup>
            </td>
        </tr>
        <tr>
            <td width="80%" class="cut-text">
                <TagsTree 
                    :tags="tags">
                </TagsTree>
            </td>
        </tr>
    </table>
</template>

<style scoped>
.instance-details-table {
    margin-top: var(--details-top-margin);
    margin-left: 5%;
    width: 95% !important;
    background-color: var(--instance-details-bg-color) !important;
    font-size: 0.8rem;
}

.instance-details-table td {
    vertical-align: top;
}

.instance-details-table>:not(caption) >* >* {
  background-color: var(--instance-details-bg-color) !important;
}

.instance-details-table >* >* {
  background-color: var(--instance-details-bg-color) !important;
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

.instance-button-group i {
    font-size: 1.2rem !important;
}
</style>

