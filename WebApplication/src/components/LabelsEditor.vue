<script>
import { mapState, mapGetters } from "vuex"
import api from "../orthancApi";
import Tags from "bootstrap5-tags/tags.js"
import LabelHelpers from "../helpers/label-helpers"


window.filterLabel = (str) => {
    return LabelHelpers.filterLabel(str);
}

export default {
    props: ['labels', 'studyId', 'id', 'title'],
    emits: ["labelsUpdated"],
    setup() {
    },
    data() {
        return {
            labelsModel: [],
            allLabelsLocalCopy: new Set()
        };
    },
    async created() {
        this.labelsModel = this.labels;
        this.allLabelsLocalCopy = this.allLabels;
    },
    async mounted() {
        Tags.init();
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
            allLabels: state => state.labels.allLabels,
        }),
        getTitle() {
            //console.log(this.title);
            if (!this.title) {
                return this.$t('labels.study_details_title');
            }
            return this.$t(this.title)
        }
    },
    components: {},
    methods: {
        hasLabel(label) {
            return this.labelsModel.includes(label);
        },
    },
    watch: {
        labelsModel: {
            async handler(oldValue, newValue) {
                if (this.studyId) {
                    let changed = await api.updateLabels({
                        studyId: this.studyId,
                        labels: this.labelsModel
                    });
                    if (changed) {
                        this.$emit("labelsUpdated");
                        setTimeout(() => { this.$store.dispatch('labels/refresh') }, 100);
                    }
                } else {
                    this.$emit("labelsUpdated", this.labelsModel);
                }
            },
            deep: true
        }
    }

}
</script>


<template>
    <div :id="id">
        <label :for="'select-' + id" class="form-label"><span v-html="getTitle"></span><span
                class="invalid-label-tips invalid-label-tips-hidden">{{ $t('labels.valid_alphabet_warning')
                }}</span>
        </label>

        <select class="form-select" :id="'select-' + id" name="tags[]" v-model="labelsModel" multiple
            data-allow-clear="true" data-allow-new="true" data-badge-style="info" data-input-filter="filterLabel"
            :placeholder="$t('labels.add_labels_placeholder')">
            <option v-for="label in allLabelsLocalCopy" :key="label" :value="label" :selected="hasLabel(label)">{{ label
                }}
            </option>
        </select>
    </div>
</template>

<style>
.invalid-label-tips {
    color: red;
    font-weight: 600;
    margin-left: 2rem;
}

.invalid-label-tips-hidden {
    display: none;
}
</style>