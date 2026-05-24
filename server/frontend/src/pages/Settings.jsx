import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Badge } from '@/components/ui/badge'

const CONFIG_ITEMS = [
  { label: 'API Endpoint',  value: 'http://localhost:5000/api/machines' },
  { label: 'Agent POST',    value: '/api/machines/report' },
  { label: 'Database',      value: 'spyware.db (SQLite)' },
  { label: 'Frontend port', value: '5173 (dev)' },
]

export default function Settings() {
  return (
    <div className="flex flex-col gap-6 p-6">
      <div>
        <h1 className="text-xl font-semibold">Settings</h1>
        <p className="text-sm text-muted-foreground">Project configuration reference</p>
      </div>

      <Card>
        <CardHeader>
          <CardTitle className="text-base">Stack</CardTitle>
          <CardDescription>Technologies used in this project</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-wrap gap-2">
          {['React 19', 'Vite 8', 'Tailwind v4', 'shadcn/ui', 'Flask 3', 'SQLite', 'libcurl', 'MinGW'].map(
            (tech) => (
              <Badge key={tech} variant="secondary">{tech}</Badge>
            )
          )}
        </CardContent>
      </Card>

      <Card>
        <CardHeader>
          <CardTitle className="text-base">Endpoints</CardTitle>
          <CardDescription>API routes configuration</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-col gap-3">
          {CONFIG_ITEMS.map(({ label, value }) => (
            <div key={label} className="flex items-center justify-between text-sm">
              <span className="text-muted-foreground">{label}</span>
              <code className="rounded bg-secondary px-2 py-0.5 text-xs font-mono">{value}</code>
            </div>
          ))}
        </CardContent>
      </Card>
    </div>
  )
}
