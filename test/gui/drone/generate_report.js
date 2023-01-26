// clang-format off
const fs = require('fs')
const path = require('path')
const process = require('process')

function getResultFile() {
  const resultDir = process.argv.pop()

  if (!resultDir) {
    throw new Error('[Error] Report directory path not provided')
  }

  const filename = 'results-v1.js'
  const resultFile = path.join(path.resolve(resultDir), filename)

  if (!fs.existsSync(resultFile)) {
    throw new Error('Report file not found: ' + resultFile)
  }
  return resultFile
}

function parse(file) {
  let content = fs.readFileSync(file, 'utf8')
  content += '\n;(()=> data)();'
  return eval(content)
}

function generateResult(scenario) {
  let result = { scenario: scenario.name }
  if (!scenario.isSkipped) {
    result.status = 'PASS'
    for (const step of scenario.tests) {
      if (step.tests && step.tests.length) {
        for (const error of step.tests) {
          if (error.result === 'ERROR' || error.result === 'FAIL') {
            result.status = 'FAIL'
          }
        }
      }
    }
  } else {
    result.status = 'Skipped'
  }
  return result
}

function generateReport(result) {
  const report = {}
  for (const suite of result.tests) {
    for (const tstCase of suite.tests) {
      const test_case_report = []
      for (const feature of tstCase.tests) {
        for (const scenario of feature.tests) {
          if (scenario.type == 'scenariooutline') {
            for (const example of scenario.tests) {
              const result = generateResult(example)
              result.example = example.name
              test_case_report.push(result)
            }
          } else {
            const result = generateResult(scenario)
            test_case_report.push(result)
          }
        }
      }
      report[tstCase.name] = test_case_report
    }
  }
  return report
}

// function textReport(report) {
//   log = ['+' + '-'.repeat(80) + '+']
//   for (const tstCase in report) {
//     log.push(`| ${tstCase}` + ' '.repeat(66) + '|')
//     log.push('+' + '-'.repeat(80) + '+')
//     report[tstCase].forEach((test, idx) => {
//       log.push(`| ${idx + 1}. | ${test.scenario} | ${test.status} |`)
//       log.push('|' + '_'.repeat(80) + '|')
//     })
//   }
//   console.log(log.join('\n'))
//   return log.join('\n')
// }

function htmlReport(report) {
  console.log(report)
  log = ['<body>']
  for (const tstCase in report) {
    report[tstCase].forEach((test, idx) => {
      //
    })
  }
  log.push('</body>')
  fs.writeFileSync(
    path.resolve(path.join('..', 'reports', 'minimal-report.html')),
    log.join('\n')
  )
}

function parseResult() {
  const resultFile = getResultFile()

  if (!fs.existsSync(resultFile)) {
    throw new Error('Report file not found: ' + resultFile)
  }
  if (!fs.lstatSync(resultFile).isFile()) {
    throw new Error('Provided path is not a file: ' + resultFile)
  }
  if (path.extname(resultFile) !== '.js') {
    // TODO: parse results from the file content
    throw new Error('Provided path is not a JS file: ' + resultFile)
  }

  const results = parse(resultFile)
  for (const result of results) {
    const report = generateReport(result)
    htmlReport(report)
  }
}

parseResult()
