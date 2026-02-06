<script>
import { Editor, EditorContent } from '@tiptap/vue-3'
import StarterKit from '@tiptap/starter-kit'
import TextAlign from '@tiptap/extension-text-align'
import Link from '@tiptap/extension-link'
import { ListKit } from '@tiptap/extension-list'

import { ref } from 'vue'

export default {
  props: {
    modelValue: {
      type: String,
      default: '',
    },
    targetUsage: {
      type: String,
      default: 'report'
    },
    insertableTexts: {
      type: Object,
      default: null
    }
  },
  emits: ['update:modelValue'],
  data() {
    return {
      content: null,
      editor: null
    };
  },
  mounted() {
    this.editor = new Editor({
      content: this.modelValue,
      extensions: [
        StarterKit,
        TextAlign.configure({
          types: ['heading', 'paragraph'],
          defaultAlignment: 'left'
        }),
        Link.configure({
          openOnClick: false, // optional, better UX in editors
        }),
        ListKit.configure({
          bulletList: true
        })
      ],
    });
    // console.log("------------", this.editor);
  },

  beforeUnmount() {
    this.editor.destroy()
  },
  methods: {
    setLink() {
      const previousUrl = this.editor.getAttributes('link').href
      const url = window.prompt('URL', previousUrl)

      if (url === null) {
        return
      }

      if (url === '') {
        this.editor.chain().focus().extendMarkRange('link').unsetLink().run()
        return
      }

      this.editor.chain().focus().extendMarkRange('link').setLink({ href: url }).run()
    },
    insertText(event, key) {
      event.preventDefault();
      this.editor.chain().focus().insertContent(this.insertableTexts[key]).run();
    }
  },
  computed: {
    hasAlignButtons() {
      return false;
    },
    hasHeadingButtons() {
      return this.targetUsage == 'report';
    },
    hasListButtons() {
      return this.targetUsage == 'report';
    },
    hasInsertButtons() {
      return this.insertableTexts != null && Object.keys(this.insertableTexts).length > 0;
    }

  },
  components: { EditorContent }
};
</script>

<template>
  <div v-if="editor" class="container-editor">
    <div class="control-group">
      <div class="btn-group" v-if="hasHeadingButtons">
        <button @click="editor.chain().focus().toggleHeading({ level: 1 }).run()"
          :class="{ 'is-active': editor.isActive('heading', { level: 1 }) }" class="btn btn-light">
          <i class="bi bi-type-h1"></i>
        </button>
        <button @click="editor.chain().focus().toggleHeading({ level: 2 }).run()"
          :class="{ 'is-active': editor.isActive('heading', { level: 2 }) }" class="btn btn-light">
          <i class="bi bi-type-h2"></i>
        </button>
        <button @click="editor.chain().focus().setParagraph().run()"
          :class="{ 'is-active': editor.isActive('paragraph') }" class="btn btn-light">
          <i class="bi bi-paragraph"></i>
        </button>
      </div>
      <div class="btn-group">
        <button @click="editor.chain().focus().toggleBold().run()" :class="{ 'is-active': editor.isActive('bold') }" class="btn btn-light">
          <i class="bi bi-type-bold"></i>
        </button>
        <button @click="editor.chain().focus().toggleItalic().run()"
          :class="{ 'is-active': editor.isActive('italic') }" class="btn btn-light">
          <i class="bi bi-type-italic"></i>
        </button>
        <!-- <button @click="editor.chain().focus().toggleStrike().run()"
          :class="{ 'is-active': editor.isActive('strike') }" class="btn btn-light">
          <i class="bi bi-type-strikethrough"></i>
        </button> -->
      </div>
      <div class="btn-group">
        <button @click="setLink" :class="{ 'is-active': editor.isActive('link') }" class="btn btn-light">
          <i class="bi bi-link"></i>
        </button>
      </div>
      <div class="btn-group" v-if="hasAlignButtons">
        <button @click="editor.chain().focus().setTextAlign('left').run()"
          :class="{ 'is-active': editor.isActive({ textAlign: 'left' }) }" class="btn btn-light">
          <i class="bi bi-text-left"></i>
        </button>
        <button @click="editor.chain().focus().setTextAlign('center').run()"
          :class="{ 'is-active': editor.isActive({ textAlign: 'center' }) }" class="btn btn-light">
          <i class="bi bi-text-center"></i>
        </button>
        <button @click="editor.chain().focus().setTextAlign('right').run()"
          :class="{ 'is-active': editor.isActive({ textAlign: 'right' }) }" class="btn btn-light">
          <i class="bi bi-text-right"></i>
        </button>
      </div>
      <div class="btn-group" v-if="hasListButtons">
        <button @click="editor.chain().focus().toggleBulletList().run()"
         :class="{ 'is-active': editor.isActive('bulletList') }" class="btn btn-light">
          <i class="bi bi-list-ul"></i>
        </button>
      </div>
      <div class="btn-group" v-if="hasInsertButtons">
        <div class="dropdown">
          <button class="btn btn-light dropdown-toggle" type="button" data-bs-toggle="dropdown" aria-expanded="false">
            Insert
          </button>
          <ul class="dropdown-menu">
            <li v-for="(v, k) in insertableTexts" :key="k"><a class="dropdown-item" href="#" @click="insertText($event, k)">{{ k + ": " + v }}</a></li>
          </ul>
        </div>
      </div>
    </div>
    <div class="editor-content">
    <editor-content v-if="editor" :editor="editor" />
    <p v-else>Editor is loading...</p>
    </div>
  </div>
</template>
<style lang="scss" scoped>
.editor-container {
  width: 100%;
  min-width: 280px;
  resize: vertical;          /* drag handle */
  overflow: hidden;          /* required for resize */
}

.editor-content :deep(.ProseMirror) {
  min-height: 150px;
  max-height: 60vh;
  overflow-y: auto;

  padding: 12px;
  box-sizing: border-box;

  border: var(--bs-border-color);
  border-width: var(--bs-border-width);
  border-style: var(--bs-border-style);
  border-radius: var(--bs-border-radius);
}



/* Basic editor styles */
.tiptap {
  :first-child {
    margin-top: 0;
  }

  /* List styles */
  ul,
  ol {
    padding: 0 1rem;
    margin: 1.25rem 1rem 1.25rem 0.4rem;

    li p {
      margin-top: 0.25em;
      margin-bottom: 0.25em;
    }
  }

  /* Heading styles */
  h1,
  h2 {
    line-height: 1.1;
    margin-top: 2.5rem;
    text-wrap: pretty;
  }

  h1,
  h2 {
    margin-top: 3.5rem;
    margin-bottom: 1.5rem;
  }

  h1 {
    font-size: 1.2rem;
  }

  h2 {
    font-size: 1.1rem;
  }


  /* Code and preformatted text styles */
  code {
    background-color: var(--purple-light);
    border-radius: 0.4rem;
    color: var(--black);
    font-size: 0.85rem;
    padding: 0.25em 0.3em;
  }

  pre {
    background: var(--black);
    border-radius: 0.5rem;
    color: var(--white);
    font-family: 'JetBrainsMono', monospace;
    margin: 1.5rem 0;
    padding: 0.75rem 1rem;

    code {
      background: none;
      color: inherit;
      font-size: 0.8rem;
      padding: 0;
    }
  }

  blockquote {
    border-left: 3px solid var(--gray-3);
    margin: 1.5rem 0;
    padding-left: 1rem;
  }

  hr {
    border: none;
    border-top: 1px solid var(--gray-2);
    margin: 2rem 0;
  }
}
</style>