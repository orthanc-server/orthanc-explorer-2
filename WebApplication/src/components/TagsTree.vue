<script>
import CopyToClipboardButton from "./CopyToClipboardButton.vue";

export default {
  props: ["tags"],
  data() {
    return {};
  },
  mounted() {},
  computed: {},
  methods: {},
  components: { CopyToClipboardButton },
};
</script>

<template>
  <div class="tags-container">
    <div v-for="(item, key) in tags" :key="key" class="tag-item">
      <!-- Sequence тип -->
      <div v-if="item.Type === 'Sequence'" class="tag-sequence">
        <div class="tag-label">{{ key }} - {{ item.Name }}</div>
        <div class="sequence-items">
          <div
            v-for="(subItem, subIndex) in item.Value"
            :key="subIndex"
            class="sequence-item"
          >
            <div class="sequence-item-header">Item {{ subIndex + 1 }}</div>
            <TagsTree :tags="subItem"></TagsTree>
          </div>
        </div>
      </div>

      <!-- Обычные теги -->
      <div v-else class="tag-field">
        <div class="tag-label">{{ key }} - {{ item.Name }}</div>
        <div class="tag-value-wrapper">
          <span v-if="item.Type === 'String'" class="tag-value">{{
            item.Value
          }}</span>
          <span v-else class="tag-value">{{ item.Value }}</span>
          <CopyToClipboardButton
            :valueToCopy="item.Value"
            class="copy-button"
          />
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.details-label {
  font-weight: 700;
  max-width: 40%;
  /* overflow: hidden;
    text-overflow: ellipsis; */
}

.details {
  margin-left: auto !important;
  font-weight: 500;
  /* overflow: hidden;
    text-overflow: ellipsis; */
}

ul {
  list-style: none;
  border-bottom-width: 1px;
  border-bottom-style: solid;
  border-bottom-color: #80808080;
}

ul li {
  display: block;
  border-bottom-width: 1px;
  border-bottom-style: solid;
  border-bottom-color: #80808080;
}

ul:last-child {
  border: none;
}

ul li:last-child {
  border: none;
}

.tags-container {
  display: flex;
  flex-direction: column;
  gap: 0;
}

.tag-item {
  padding: 0.75rem 0;
  border-bottom: 1px solid #e9ecef;
}

.tag-item:last-child {
  border-bottom: none;
}

.tag-field {
  display: flex;
  flex-direction: column;
  gap: 0.25rem;
}

.tag-label {
  font-size: 0.6875rem;
  font-weight: 600;
  color: #6c757d;
  text-transform: uppercase;
  letter-spacing: 0.3px;
  line-height: 1.2;
}

.tag-value-wrapper {
  display: flex;
  align-items: center;
  gap: 0.5rem;
}

.tag-value {
  font-size: 0.875rem;
  color: #212529;
  word-break: break-word;
  flex: 1;
  line-height: 1.5;
}

.copy-button {
  flex-shrink: 0;
}

/* Стили для Sequence */
.tag-sequence {
  display: flex;
  flex-direction: column;
  gap: 0.5rem;
}

.sequence-items {
  padding-left: 1rem;
  display: flex;
  flex-direction: column;
  gap: 0.75rem;
}

.sequence-item {
  background: #f8f9fa;
  border: 1px solid #dee2e6;
  border-radius: 0.25rem;
  padding: 0.75rem;
}

.sequence-item-header {
  font-weight: 600;
  font-size: 0.8125rem;
  color: #495057;
  margin-bottom: 0.5rem;
}
</style>
