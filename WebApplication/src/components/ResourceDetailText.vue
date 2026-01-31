<script>
import CopyToClipboardButton from "./CopyToClipboardButton.vue";
import { translateDicomTag } from "../locales/i18n";
import dateHelpers from "../helpers/date-helpers";
import { mapState, mapGetters } from "vuex";

export default {
  props: ["truncate", "copy", "tags", "tag", "showIfEmpty"], // you can set copy=false if you don't wan't the copy button !
  setup() {
    return {};
  },
  data() {
    return {};
  },
  mounted() {},
  methods: {},
  computed: {
    ...mapState({
      uiOptions: (state) => state.configuration.uiOptions,
    }),
    title() {
      return translateDicomTag(this.$i18n.t, this.$i18n.te, this.tag);
    },
    value() {
      if (this.hasValue) {
        let value = this.tags[this.tag];
        if (dateHelpers.isDateTag(this.tag)) {
          return dateHelpers.formatDateForDisplay(
            value,
            this.uiOptions.DateFormat,
          );
        } else {
          return value;
        }
      } else {
        return "";
      }
    },
    hasValue() {
      return this.tag in this.tags;
    },
    showValue() {
      return this.showIfEmpty || this.hasValue;
    },
  },
  components: { CopyToClipboardButton },
};
</script>

<template>
  <div v-if="showValue" class="detail-item col-auto px-1">
    <label
      class="detail-label"
      :class="{ 'text-truncate': this.truncate }"
      data-bs-toggle="tooltip"
      v-bind:title="title"
    >
      {{ title }}
    </label>
    <div class="d-flex align-items-start">
      <div
        class="detail-value flex-grow-1"
        data-bs-toggle="tooltip"
        v-bind:title="value"
      >
        {{ value || "-" }}
      </div>
      <CopyToClipboardButton
        v-if="!this.copy && value && value.length > 0"
        :valueToCopy="value"
        class="flex-shrink-0 ms-2"
      />
    </div>
  </div>
</template>

<style>
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
.detail-item {
  padding: 0.625rem 0;
  border-bottom: 1px solid #dee2e6;
}

.detail-item:last-child {
  border-bottom: none;
  padding-bottom: 0;
}

.detail-label {
  display: block;
  font-size: 11px;
  font-weight: 600;
  color: #6c757d;
  text-transform: uppercase;
  letter-spacing: 0.3px;
  margin-bottom: 0.25rem;
  line-height: 1.2;
}

.detail-value {
  font-size: 14px;
  color: #212529;
  word-break: break-word;
  line-height: 1.4;
  min-height: 1.4em;
}
</style>
