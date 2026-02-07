<script>
import CopyToClipboardButton from "./CopyToClipboardButton.vue";

export default {
  name: "TagsTree",
  props: {
    tags: {
      type: Object,
      default: () => ({})
    },
    showEmpty: {
      type: Boolean,
      default: false
    },
    isNested: {
      type: Boolean,
      default: false
    }
  },
  data() {
    return {
      expandedSequences: {}  // Для отслеживания развёрнутых Sequence
    };
  },
  computed: {
    filteredTags() {
      if (!this.tags) return {};
      if (this.showEmpty) return this.tags;

      const filtered = {};
      for (const [key, item] of Object.entries(this.tags)) {
        if (!item) continue;

        if (item.Type === 'Sequence') {
          if (item.Value && Array.isArray(item.Value) && item.Value.length > 0) {
            filtered[key] = item;
          }
        } else {
          const value = item.Value;
          if (value !== null && value !== undefined && String(value).trim() !== '') {
            filtered[key] = item;
          }
        }
      }
      return filtered;
    },
    // Разделяем теги на обычные и Sequence
    regularTags() {
      const result = {};
      for (const [key, item] of Object.entries(this.filteredTags)) {
        if (item.Type !== 'Sequence') {
          result[key] = item;
        }
      }
      return result;
    },
    sequenceTags() {
      const result = {};
      for (const [key, item] of Object.entries(this.filteredTags)) {
        if (item.Type === 'Sequence') {
          result[key] = item;
        }
      }
      return result;
    },
    hasRegularTags() {
      return Object.keys(this.regularTags).length > 0;
    },
    hasSequenceTags() {
      return Object.keys(this.sequenceTags).length > 0;
    }
  },
  methods: {
    toggleSequence(key) {
      this.expandedSequences[key] = !this.expandedSequences[key];
    },
    isSequenceExpanded(key) {
      return this.expandedSequences[key] ?? false;
    },
    formatTagKey(key) {
      // Форматируем ключ для лучшего отображения (0008,0018 -> 0008,0018)
      return key;
    }
  },
  components: { CopyToClipboardButton },
};
</script>

<template>
  <div class="tags-tree" :class="{ 'tags-tree--nested': isNested }">
    <!-- Обычные теги в виде карточек -->
    <div v-if="hasRegularTags" class="tags-grid">
      <div v-for="(item, key) in regularTags" :key="key" class="tag-card">
        <div class="tag-card__header">
          <span class="tag-card__key">{{ formatTagKey(key) }}</span>
          <span class="tag-card__name">{{ item.Name }}</span>
        </div>
        <div class="tag-card__body">
          <span class="tag-card__value" :title="String(item.Value)">
            {{ item.Value }}
          </span>
          <CopyToClipboardButton :valueToCopy="String(item.Value)" class="tag-card__copy" />
        </div>
      </div>
    </div>

    <!-- Sequence теги -->
    <div v-if="hasSequenceTags" class="sequences-container">
      <div v-for="(item, key) in sequenceTags" :key="key" class="sequence-card"
        :class="{ 'sequence-card--expanded': isSequenceExpanded(key) }">
        <!-- Заголовок Sequence -->
        <div class="sequence-card__header" @click="toggleSequence(key)">
          <div class="sequence-card__title">
            <i class="bi" :class="isSequenceExpanded(key) ? 'bi-chevron-down' : 'bi-chevron-right'"></i>
            <span class="sequence-card__key">{{ formatTagKey(key) }}</span>
            <span class="sequence-card__name">{{ item.Name }}</span>
          </div>
          <span class="sequence-card__count">
            {{ item.Value.length }} {{ item.Value.length === 1 ? 'item' : 'items' }}
          </span>
        </div>

        <!-- Содержимое Sequence (Items) -->
        <div v-show="isSequenceExpanded(key)" class="sequence-card__content">
          <div v-for="(subItem, subIndex) in item.Value" :key="subIndex" class="sequence-item">
            <div class="sequence-item__header">
              <i class="bi bi-folder2-open"></i>
              Item {{ subIndex + 1 }}
            </div>
            <div class="sequence-item__body">
              <TagsTree :tags="subItem" :showEmpty="showEmpty" :isNested="true" />
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- Пустое состояние -->
    <div v-if="!hasRegularTags && !hasSequenceTags" class="tags-empty">
      <i class="bi bi-inbox"></i>
      <span>No tags to display</span>
    </div>
  </div>
</template>

<style scoped>
.tags-tree {
  width: 100%;
}

.tags-tree--nested {
  padding-left: 0;
}

/* ============================================
   GRID для обычных тегов
   ============================================ */
.tags-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(220px, 1fr));
  gap: 12px;
  margin-bottom: 16px;
}

/* ============================================
   Карточка тега
   ============================================ */
.tag-card {
  background: #ffffff;
  border: 1px solid #e2e8f0;
  border-radius: 8px;
  padding: 10px 12px;
  transition: all 0.2s ease;
  display: flex;
  flex-direction: column;
  gap: 6px;
  min-width: 0;
  /* Для корректного text-overflow */
}

.tag-card:hover {
  border-color: #3b82f6;
  box-shadow: 0 2px 8px rgba(59, 130, 246, 0.12);
}

.tag-card__header {
  display: flex;
  flex-direction: column;
  gap: 2px;
}

.tag-card__key {
  font-size: 10px;
  font-family: 'Monaco', 'Menlo', 'Consolas', monospace;
  color: #94a3b8;
  letter-spacing: 0.5px;
}

.tag-card__name {
  font-size: 11px;
  font-weight: 600;
  color: #64748b;
  text-transform: uppercase;
  letter-spacing: 0.3px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.tag-card__body {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 8px;
}

.tag-card__value {
  font-size: 13px;
  font-weight: 500;
  color: #1e293b;
  word-break: break-word;
  line-height: 1.4;
  flex: 1;
  min-width: 0;
  /* Ограничение на 3 строки */
  display: -webkit-box;
  -webkit-line-clamp: 3;
  -webkit-box-orient: vertical;
  overflow: hidden;
}

.tag-card__copy {
  flex-shrink: 0;
  opacity: 0;
  transition: opacity 0.2s ease;
}

.tag-card:hover .tag-card__copy {
  opacity: 1;
}

/* ============================================
   Контейнер для Sequence
   ============================================ */
.sequences-container {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

/* ============================================
   Карточка Sequence
   ============================================ */
.sequence-card {
  background: #f8fafc;
  border: 1px solid #e2e8f0;
  border-radius: 10px;
  overflow: hidden;
  transition: all 0.2s ease;
}

.sequence-card:hover {
  border-color: #cbd5e1;
}

.sequence-card--expanded {
  border-color: #3b82f6;
  box-shadow: 0 2px 12px rgba(59, 130, 246, 0.1);
}

.sequence-card__header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 12px 16px;
  cursor: pointer;
  user-select: none;
  transition: background 0.2s ease;
}

.sequence-card__header:hover {
  background: #f1f5f9;
}

.sequence-card__title {
  display: flex;
  align-items: center;
  gap: 10px;
  min-width: 0;
}

.sequence-card__title i {
  font-size: 14px;
  color: #3b82f6;
  transition: transform 0.2s ease;
}

.sequence-card__key {
  font-size: 10px;
  font-family: 'Monaco', 'Menlo', 'Consolas', monospace;
  color: #94a3b8;
  flex-shrink: 0;
}

.sequence-card__name {
  font-size: 12px;
  font-weight: 600;
  color: #475569;
  text-transform: uppercase;
  letter-spacing: 0.3px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.sequence-card__count {
  font-size: 11px;
  font-weight: 500;
  color: #fff;
  background: linear-gradient(135deg, #3b82f6, #2563eb);
  padding: 4px 10px;
  border-radius: 12px;
  flex-shrink: 0;
}

.sequence-card__content {
  border-top: 1px solid #e2e8f0;
  background: #ffffff;
}

/* ============================================
   Item внутри Sequence
   ============================================ */
.sequence-item {
  border-bottom: 1px solid #f1f5f9;
}

.sequence-item:last-child {
  border-bottom: none;
}

.sequence-item__header {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 10px 16px;
  background: linear-gradient(90deg, #f8fafc, #ffffff);
  font-size: 12px;
  font-weight: 600;
  color: #64748b;
  border-bottom: 1px solid #f1f5f9;
}

.sequence-item__header i {
  color: #f59e0b;
  font-size: 14px;
}

.sequence-item__body {
  padding: 12px 16px;
}

/* Вложенные tags-grid внутри sequence */
.sequence-item__body .tags-grid {
  grid-template-columns: repeat(auto-fill, minmax(180px, 1fr));
  gap: 8px;
  margin-bottom: 8px;
}

.sequence-item__body .tag-card {
  padding: 8px 10px;
  background: #f8fafc;
}

.sequence-item__body .tag-card__value {
  font-size: 12px;
}

/* ============================================
   Пустое состояние
   ============================================ */
.tags-empty {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 32px;
  color: #94a3b8;
  gap: 8px;
}

.tags-empty i {
  font-size: 32px;
}

.tags-empty span {
  font-size: 13px;
}

/* ============================================
   Адаптивность
   ============================================ */
@media (max-width: 768px) {
  .tags-grid {
    grid-template-columns: repeat(auto-fill, minmax(160px, 1fr));
    gap: 8px;
  }

  .tag-card {
    padding: 8px 10px;
  }

  .tag-card__name {
    font-size: 10px;
  }

  .tag-card__value {
    font-size: 12px;
  }

  .sequence-card__header {
    padding: 10px 12px;
  }

  .sequence-item__body {
    padding: 10px 12px;
  }
}

@media (max-width: 480px) {
  .tags-grid {
    grid-template-columns: 1fr;
  }

  .sequence-item__body .tags-grid {
    grid-template-columns: 1fr;
  }
}
</style>