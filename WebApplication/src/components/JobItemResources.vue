<script>
import CopyToClipboardButton from "./CopyToClipboardButton.vue";
import { translateDicomTag } from "../locales/i18n"
import dateHelpers from "../helpers/date-helpers"
import { mapState, mapGetters } from "vuex"
import orthancApi from "../orthancApi";
import resourceHelpers from "../helpers/resource-helpers";


export default {
    props: ["resources"],
    setup() {
        return {
        }
    },
    data() {
        return {
            resources_: []
        }
    },
    async mounted() {
        this.resources_ = [];
        for (let r of this.resources) {
            let rr = {
                Type: r.Type,
                ID: r.ID,
                StudyInstanceUID: null,
                Display: r.Type + ' ' + r.ID
            }
            if (rr.Type == 'Study') {
                try {
                    let study = await orthancApi.getStudy(rr.ID);
                    rr.Display = resourceHelpers.getResourceTitle("study", study.PatientMainDicomTags, study.MainDicomTags, {}, {});
                    rr.StudyInstanceUID = study.MainDicomTags.StudyInstanceUID;
                } catch (err) { // ignore (ex: resource not found)
                }
            }
            this.resources_.push(rr);
        }
    },
    methods: {
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
        })
    },
}
</script>

<template>
    <div class="container">
        <div class="row"><span class="text-start w-100"><strong>{{ $t('jobs.title_resources') }}:</strong></span></div>
        <div class="row" v-for="r in resources_">
            <span v-if="r.StudyInstanceUID" class="text-start">
                <router-link 
                    v-bind:to="'/filtered-studies?StudyInstanceUID=' + r.StudyInstanceUID + '&expand=study'"
                    >{{ r.Display }}</router-link>
            </span>
            <span v-else>
                {{ r.Display }}
            </span>
        </div>
    </div>
</template>

<style></style>
