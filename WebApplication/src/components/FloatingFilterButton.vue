<script>
import { mapState } from "vuex";

export default {
    name: "FloatingFilterButton",

    props: {
        isModalOpen: {
            type: Boolean,
            default: false,
        },
    },

    emits: ["open-filters"],

    computed: {
        ...mapState({
            dicomTagsFilters: (state) => state.studies.dicomTagsFilters,
            labelFilters: (state) => state.studies.labelFilters,
            isSearching: (state) => state.studies.isSearching,
        }),

        hasActiveFilters() {
            const filters = this.dicomTagsFilters || {};

            for (const [key, value] of Object.entries(filters)) {
                if (value === "NONE" || value === null || value === undefined) continue;
                if (typeof value === "string" && value.length > 0) return true;
                if (Array.isArray(value) && value.length > 0) return true;
            }

            if (this.dicomTagsFilters?.StudyDate) return true;
            if (Array.isArray(this.labelFilters) && this.labelFilters.length > 0) return true;

            return false;
        },

        filterCount() {
            let count = 0;
            const filters = this.dicomTagsFilters || {};

            for (const [key, value] of Object.entries(filters)) {
                if (value === "NONE" || value === null || value === undefined) continue;
                if (typeof value === "string" && value.length > 0) count++;
                if (Array.isArray(value) && value.length > 0) count++;
            }

            if (this.dicomTagsFilters?.StudyDate) count++;
            if (Array.isArray(this.labelFilters) && this.labelFilters.length > 0) {
                count += this.labelFilters.length;
            }

            return count;
        },
    },

    methods: {
        openFilters() {
            this.$emit("open-filters");
        },
    },
};
</script>

<template>
    <transition name="fab">
        <button class="floating-filter-btn" :class="{
            'has-filters': hasActiveFilters,
            'is-open': isModalOpen,
            'is-searching': isSearching,
        }" @click="openFilters" :title="$t('mobile_menu.filters') || 'Фильтры'">
            <i class="bi bi-funnel-fill filter-icon"></i>

            <transition name="badge">
                <span v-if="hasActiveFilters" class="filter-badge">
                    {{ filterCount }}
                </span>
            </transition>

            <div v-if="hasActiveFilters" class="pulse-ring"></div>
        </button>
    </transition>
</template>

<style scoped>
.floating-filter-btn {
    position: fixed;
    right: 16px;
    bottom: 80px;
    z-index: 9998;
    width: 56px;
    height: 56px;
    background: linear-gradient(135deg, #3b82f6 0%, #2563eb 100%);
    border: none;
    border-radius: 50%;
    box-shadow:
        0 4px 12px rgba(59, 130, 246, 0.4),
        0 8px 24px rgba(59, 130, 246, 0.2);
    display: flex;
    align-items: center;
    justify-content: center;
    color: white;
    cursor: pointer;
    transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
    transform: scale(1);
}

.floating-filter-btn:hover {
    transform: scale(1.05);
    box-shadow:
        0 6px 16px rgba(59, 130, 246, 0.5),
        0 12px 32px rgba(59, 130, 246, 0.3);
}

.floating-filter-btn:active {
    transform: scale(0.95);
}

.floating-filter-btn.is-open {
    background: linear-gradient(135deg, #2563eb 0%, #1e40af 100%);
    transform: rotate(180deg) scale(1);
}

.floating-filter-btn.is-open:hover {
    transform: rotate(180deg) scale(1.05);
}

.floating-filter-btn.has-filters {
    background: linear-gradient(135deg, #10b981 0%, #059669 100%);
}

.floating-filter-btn.has-filters:hover {
    background: linear-gradient(135deg, #059669 0%, #047857 100%);
}

.floating-filter-btn.is-searching {
    animation: searching-pulse 1.5s ease-in-out infinite;
}

@keyframes searching-pulse {

    0%,
    100% {
        box-shadow:
            0 4px 12px rgba(59, 130, 246, 0.4),
            0 8px 24px rgba(59, 130, 246, 0.2);
    }

    50% {
        box-shadow:
            0 6px 20px rgba(59, 130, 246, 0.6),
            0 12px 40px rgba(59, 130, 246, 0.4);
    }
}

.filter-icon {
    font-size: 24px;
    transition: transform 0.3s cubic-bezier(0.4, 0, 0.2, 1);
}

.floating-filter-btn.is-open .filter-icon {
    transform: rotate(180deg);
}

.filter-badge {
    position: absolute;
    top: -4px;
    right: -4px;
    min-width: 20px;
    height: 20px;
    padding: 0 5px;
    background: linear-gradient(135deg, #ef4444 0%, #dc2626 100%);
    color: white;
    font-size: 11px;
    font-weight: 700;
    border-radius: 10px;
    border: 2px solid #0f172a;
    display: flex;
    align-items: center;
    justify-content: center;
    box-shadow: 0 2px 8px rgba(239, 68, 68, 0.5);
}

.pulse-ring {
    position: absolute;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    width: 100%;
    height: 100%;
    border: 2px solid #10b981;
    border-radius: 50%;
    opacity: 0;
    animation: pulse-ring 2s cubic-bezier(0.4, 0, 0.6, 1) infinite;
}

@keyframes pulse-ring {
    0% {
        transform: translate(-50%, -50%) scale(1);
        opacity: 0.8;
    }

    100% {
        transform: translate(-50%, -50%) scale(1.4);
        opacity: 0;
    }
}

.fab-enter-active,
.fab-leave-active {
    transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
}

.fab-enter-from {
    transform: scale(0) rotate(-180deg);
    opacity: 0;
}

.fab-leave-to {
    transform: scale(0) rotate(180deg);
    opacity: 0;
}

.badge-enter-active,
.badge-leave-active {
    transition: all 0.2s ease;
}

.badge-enter-from,
.badge-leave-to {
    transform: scale(0);
    opacity: 0;
}

@supports (bottom: env(safe-area-inset-bottom)) {
    .floating-filter-btn {
        bottom: calc(80px + env(safe-area-inset-bottom, 0));
    }
}

@media (max-width: 360px) {
    .floating-filter-btn {
        width: 52px;
        height: 52px;
        right: 12px;
        bottom: 76px;
    }

    .filter-icon {
        font-size: 22px;
    }

    .filter-badge {
        min-width: 18px;
        height: 18px;
        font-size: 10px;
    }
}
</style>