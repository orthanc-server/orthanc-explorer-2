<script>
import CopyToClipboardButton from "./CopyToClipboardButton.vue";
import { translateDicomTag } from "../locales/i18n";
import dateHelpers from "../helpers/date-helpers";
import { mapState, mapGetters } from "vuex";

export default {
  props: ["truncate", "copy", "tags", "tag", "showIfEmpty"],
  data() {
    return {};
  },
  computed: {
    ...mapState({
      uiOptions: (state) => state.configuration.uiOptions,
    }),
    title() {
      return translateDicomTag(this.$i18n.t, this.$i18n.te, this.tag);
    },
    rawValue() {
      // Сырое значение из tags
      if (this.tag in this.tags) {
        return this.tags[this.tag];
      }
      return null;
    },
    hasValue() {
      // Проверяем что значение реально существует и не пустое
      const val = this.rawValue;
      if (val === null || val === undefined) return false;
      if (typeof val === 'string' && val.trim() === '') return false;
      return true;
    },
    value() {
      if (!this.hasValue) return "";

      const val = this.rawValue;
      if (dateHelpers.isDateTag(this.tag)) {
        return dateHelpers.formatDateForDisplay(val, this.uiOptions.DateFormat);
      }
      return val;
    },
    showValue() {
      // Показываем только если:
      // 1. showIfEmpty = true (принудительно показать даже пустые)
      // 2. ИЛИ есть реальное значение
      return this.showIfEmpty || this.hasValue;
    },
  },
  components: { CopyToClipboardButton },
};
</script>

<template>
  <div v-if="showValue" class="detail-item col-auto px-1">
    <label class="detail-label" :class="{ 'text-truncate': truncate }" data-bs-toggle="tooltip" :title="title">
      {{ title }}
    </label>
    <div class="d-flex align-items-start">
      <div class="detail-value flex-grow-1" data-bs-toggle="tooltip" :title="value">
        {{ value || "-" }}
      </div>
      <CopyToClipboardButton v-if="copy !== false && value && value.length > 0" :valueToCopy="value"
        class="flex-shrink-0 ms-2" />
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
