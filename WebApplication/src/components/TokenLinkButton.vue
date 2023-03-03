<script>
import Modal from "./Modal.vue"
import ShareModal from "./ShareModal.vue"
import ModifyModal from "./ModifyModal.vue"
import $ from "jquery"
import { mapState } from "vuex"
import api from "../orthancApi"
import resourceHelpers from "../helpers/resource-helpers"
import clipboardHelpers from "../helpers/clipboard-helpers"


export default {
    props: ["resourcesOrthancId", "linkUrl", "level", "tokenType", "validityDuration", "title", "iconClass", "opensInNewTab"],
    setup() {
        return {
        }
    },

    data() {
        return {
        };
    },
    mounted() {
    },
    methods: {
        async clicked(event) {
            event.preventDefault();
            let validityDuration = this.validityDuration;
            if (validityDuration == null || validityDuration === undefined) {
                validityDuration = this.tokens.InstantLinksValidity;
            }
            let token = await api.createToken({tokenType: this.tokenType, resourcesIds: this.resourcesOrthancId, level: this.level, validityDuration: validityDuration});
            let finalUrl = this.linkUrl;

            // if (token["Url"] != null) {
            //     finalUrl = token["Url"];
            // }
            // else 
            if (this.linkUrl.indexOf('?') == -1) {
                finalUrl = finalUrl + "?token=" + token["Token"];
            } else {
                finalUrl = finalUrl + "&token=" + token["Token"];
            }

            let link = document.querySelector("#hidden-link-for-tokens");
            if (link == null) {
                link = document.createElement('a');
                document.body.appendChild(link);
                link.setAttribute("id", "hidden-link-for-tokens");
                link.setAttribute("type", "hidden");
            }
            link.target = this.target;
            link.href = finalUrl;
            link.click();
        },
    },
    computed: {
        ...mapState({
            tokens: state => state.configuration.tokens,
        }),
        target() {
            if (this.opensInNewTab === undefined || this.opensInNewTab == false) {
                return "";
            } else {
                return "blank";
            }
        }
    },
    components: {  }
}
</script>

<template>
    <div>
    <a v-if="tokens.RequiredForLinks"
        class="btn btn-sm btn-secondary m-1" type="button" data-bs-toggle="tooltip"
        :title="title" @click="clicked"
        href="#">
        <i :class="iconClass"></i>
    </a>
    <a v-if="!tokens.RequiredForLinks"
        class="btn btn-sm btn-secondary m-1" type="button" data-bs-toggle="tooltip"
        :title="title" :target="target"
        :href="linkUrl">
        <i :class="iconClass"></i>
    </a>
    </div>
</template>

<style>
</style>