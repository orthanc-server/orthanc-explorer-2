<script>
import ResourceButtonGroup from "./ResourceButtonGroup.vue";
import InstanceList from "./InstanceList.vue";
import ResourceDetailText from "./ResourceDetailText.vue";
import { mapState, mapGetters } from "vuex"

export default {
    props: ['seriesId', 'seriesMainDicomTags', 'studyMainDicomTags', 'patientMainDicomTags', 'instancesIds'],
    emits: ['deletedSeries'],
    setup() {
    },
    data() {
        return {
        };
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
        }),
    },
    components: { ResourceButtonGroup, InstanceList, ResourceDetailText },
    methods: {
        onDeletedInstance(instanceId) {
            const pos = this.instancesIds.indexOf(instanceId);
            if (pos >= 0) {
                this.instancesIds.splice(pos, 1);
                if (this.instancesIds.length == 0) {
                    this.$emit("deletedSeries", this.seriesId);
                }
            }
        },
        onDeletedSeries() {
            this.$emit("deletedSeries", this.seriesId);
        }
    }

}
</script>


<template>
    <table class="table table-responsive table-sm series-details-table">
        <tr>
            <td width="70%" class="cut-text">
                <ul>
                    <ResourceDetailText v-for="tag in uiOptions.SeriesMainTags" :key="tag" :tags="seriesMainDicomTags" :tag="tag" :showIfEmpty="true"></ResourceDetailText>
                </ul>
            </td>
            <td width="30%" class="series-button-group">
                <ResourceButtonGroup
                :resourceOrthancId="this.seriesId"
                :resourceLevel="'series'"
                :resourceDicomUid="this.seriesMainDicomTags.SeriesInstanceUID"
                :studyMainDicomTags="this.studyMainDicomTags"
                :seriesMainDicomTags="this.seriesMainDicomTags"
                :patientMainDicomTags="this.patientMainDicomTags"
                :customClass="'instance-button-group'"
                @deletedResource="onDeletedSeries"
                ></ResourceButtonGroup>
            </td>
        </tr>
        <tr>
            <td colspan="100">
                <InstanceList
                    :seriesId="this.seriesId"
                    :seriesMainDicomTags="this.seriesMainDicomTags"
                    :patientMainDicomTags="this.patientMainDicomTags"
                    :studyMainDicomTags="this.studyMainDicomTags"
                    :instancesIds="this.instancesIds"
                    @deletedInstance="onDeletedInstance"
                ></InstanceList>
            </td>
        </tr>
    </table>
</template>

<style scoped>
.series-details-table {
    margin-top: var(--details-top-margin);
    margin-left: 5%;
    width: 95% !important;
    background-color: var(--series-selected-color);
    font-size: 0.8rem;
}

.series-details-table td {
    vertical-align: top;
}

</style>
<style>
.series-button-group i {
    font-size: 1.3rem;
}
</style>