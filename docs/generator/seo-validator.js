'use strict'
/* Copyright (c) 2018 Matthew Setter <matthew@matthewsetter.com>.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * Scans converted pages for missing SEO (keyword and description) metadata. 
 * Any pages that don't contain one or both of these tags will be output to the console.
 *
 * Usage (from root of playbook repository):
 *
 *  $ NODE_PATH=netlify/node_modules antora --pull --generator=./generator/xref-validator.js antora-playbook.yml
 */
const aggregateContent = require('@antora/content-aggregator')
const buildPlaybook = require('@antora/playbook-builder')
const classifyContent = require('@antora/content-classifier')
const { convertDocument } = require('@antora/document-converter')
const { resolveConfig: resolveAsciiDocConfig } = require('@antora/asciidoc-loader')

const META_SEO_RX = /<meta name="(keywords|description)" content=".{1,}"([\s\/]+)?>/g

module.exports = async (args, env) => {
  const playbook = buildPlaybook(args, env)
  const contentCatalog = await aggregateContent(playbook).then((aggregate) => classifyContent(playbook, aggregate))
  const asciidocConfig = resolveAsciiDocConfig(playbook)
  const docsWithMissingSeoMetatags = new Map()
  const unsilenceStderr = silenceStderr()
  contentCatalog
    .getFiles()
    .filter((file) => (file.src.family === 'page' && file.out) || file.src.family === 'nav')
      .forEach((doc) => {
        convertDocument(doc, contentCatalog, asciidocConfig)
        const contents = doc.contents.toString()
        if (META_SEO_RX.test(contents)) {
          docsWithMissingSeoMetatags.set(doc)
        }
      })
  unsilenceStderr()
  if (docsWithMissingSeoMetatags.size) {
    console.log("Number of files with missing SEO tags: %d", docsWithMissingSeoMetatags.size)
    console.log(docsWithMissingSeoMetatags)
    const byOrigin = Array.from(docsWithMissingSeoMetatags).reduce((accum, page) => {
      let origin
      console.log(page)
      throw "Exit"
      const originData = page.src.origin
      let startPath = ''
      if (originData.worktree) {
        origin = [
          `worktree: ${originData.editUrlPattern.slice(7, originData.editUrlPattern.length - 3)}`,
          `component: ${page.src.component}`,
          `version: ${page.src.version}`,
        ].join(' | ')
      } else {
        if (originData.startPath) startPath = `${originData.startPath}/`
        origin = [
          `repo: ${originData.url.split(':').pop().replace(/\.git$/, '')}`,
          `branch: ${originData.branch}`,
          `component: ${page.src.component}`,
          `version: ${page.src.version}`,
        ].join(' | ')
      }
      if (!(origin in accum)) accum[origin] = []
      accum[origin].push({ path: `${startPath}${page.path}` })
      return accum
    }, {})
    console.error('Missing SEO Tags Detected:')
    console.error()
    /*Object.keys(byOrigin).sort().forEach((origin) => {
      console.error(origin)
      byOrigin[origin].sort((a, b) => a.path.localeCompare(b.path)).forEach(({ path, xrefs }) => {
        xrefs.forEach((xref) => console.error(`  path: ${path} | xref: ${xref}`))
      })
      console.error()
    })*/
    console.error('antora: SEO validation failed! See previous report for details.')
    process.exitCode = 1
  }
}

function silenceStderr () {
  const stderrWriter = process.stderr.write
  process.stderr.write = () => {}
  return () => { process.stderr.write = stderrWriter }
}

