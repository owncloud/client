'use strict'

const _ = require('lodash')
const cheerio = require('cheerio')
const Entities = require('html-entities').AllHtmlEntities
const Elasticsearch = require('elasticsearch')

const aggregateContent = require('@antora/content-aggregator')
const buildNavigation = require('@antora/navigation-builder')
const buildPlaybook = require('@antora/playbook-builder')
const classifyContent = require('@antora/content-classifier')
const convertDocuments = require('@antora/document-converter')
const createPageComposer = require('@antora/page-composer')
const loadUi = require('@antora/ui-loader')
const mapSite = require('@antora/site-mapper')
const produceRedirects = require('@antora/redirect-producer')
const publishSite = require('@antora/site-publisher')
const { resolveConfig: resolveAsciiDocConfig } = require('@antora/asciidoc-loader')

async function generateSite (args, env) {
  const playbook = buildPlaybook(args, env)
  const [contentCatalog, uiCatalog] = await Promise.all([
    aggregateContent(playbook).then((contentAggregate) => classifyContent(playbook, enforceEditurl(contentAggregate))),
    loadUi(playbook),
  ])
  const asciidocConfig = resolveAsciiDocConfig(playbook)
  const pages = convertDocuments(contentCatalog, asciidocConfig)
  const navigationCatalog = buildNavigation(contentCatalog, asciidocConfig)
  const composePage = createPageComposer(playbook, contentCatalog, uiCatalog, env)
  pages.forEach((page) => composePage(page, contentCatalog, navigationCatalog))
  const siteFiles = mapSite(playbook, pages).concat(produceRedirects(playbook, contentCatalog))
  generateIndex(playbook, pages)
  if (playbook.site.url) siteFiles.push(composePage(create404Page()))
  const siteCatalog = { getFiles: () => siteFiles }
  return publishSite(playbook, [contentCatalog, uiCatalog, siteCatalog])
}

function generateIndex (playbook, pages) {
  if (!process.env.ELASTICSEARCH_HOST || !process.env.ELASTICSEARCH_INDEX) {
    return
  }

  let siteUrl = playbook.site.url

  const documents = pages.map((page) => {
    const entities = new Entities()
    const titles = []

    const html = page.contents.toString()
    const $ = cheerio.load(html)

    const $article = $('article')
    const title = $article.find('h1').text()

    $article.find('h1,h2,h3,h4,h5,h6').each(function () {
      let $title = $(this)
      let id = $title.attr('id')

      titles.push({
        text: $title.text(),
        id: $title.attr('id'),
        url: siteUrl + page.pub.url + '#' + $title.attr('id')
      })

      $title.remove()
    })

    let text = entities.decode($('article').text())
      .replace(/(<([^>]+)>)/ig, '')
      .replace(/\n/g, ' ')
      .replace(/\r/g, ' ')
      .replace(/\s+/g, ' ')
      .trim()

    return {
      component: page.src.component,
      version: page.src.version,
      name: page.src.stem,
      title: title,
      text: text,
      url: siteUrl + page.pub.url,
      titles: titles
    }
  })

  let result = []

  documents.forEach((document, index) => {
    result.push({
      index:  {
        _index: process.env.ELASTICSEARCH_INDEX,
        _type: 'page',
        _id: index
      }
    })

    result.push(document)
  })

  const client = new Elasticsearch.Client({
    host: [{
      host: process.env.ELASTICSEARCH_HOST,
      port: process.env.ELASTICSEARCH_PORT || 443,
      protocol: process.env.ELASTICSEARCH_PROTOCOL || 'https',
      auth: process.env.ELASTICSEARCH_WRITE_AUTH,
    }]
  })

  client.deleteByQuery({
    index: process.env.ELASTICSEARCH_INDEX,
    body: {
      query: {
        term: {
          type: 'page'
        }
      }
    }
  }, function (err, resp) {
    if (err) {
      console.log('Failed to delete index:', err)
      process.exit(1)
    }
  });

  client.bulk({
    body: result
  }, function (err, resp) {
    if (err) {
      console.log('Failed to upload index:', err)
      process.exit(2)
    }
  });
}

function enforceEditurl (contentAggregate) {
  _.map(contentAggregate, (source) => {
    _.map(source.files, (file) => {
      if (_.startsWith(file.src.editUrl, 'file://')) {
        if (source.name === 'server') {
          file.src.editUrl = 'https://github.com/owncloud/docs/edit/' + source.version + '/' + file.src.path
        }
      }

      return file
    })

    return source
  })

  return contentAggregate
}

function create404Page () {
  return {
    title: 'Page Not Found',
    mediaType: 'text/html',
    src: { stem: '404' },
    out: { path: '404.html' },
    pub: { url: '/404.html', rootPath: '' },
  }
}

module.exports = generateSite
