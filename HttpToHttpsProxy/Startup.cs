using Microsoft.AspNetCore.Builder;
using Microsoft.AspNetCore.Hosting;
using Microsoft.AspNetCore.Http;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Hosting;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.Routing;
using System.Net.Http;
using System.Threading;
using System.IO;

namespace HttpToHttpsProxy
{
    public class Startup
    {
        public static HttpClient httpClient = new HttpClient();

        // This method gets called by the runtime. Use this method to add services to the container.
        // For more information on how to configure your application, visit https://go.microsoft.com/fwlink/?LinkID=398940
        public void ConfigureServices(IServiceCollection services)
        {
            services.AddMvc();
        }

        // This method gets called by the runtime. Use this method to configure the HTTP request pipeline.
        public void Configure(IApplicationBuilder app, IWebHostEnvironment env)
        {
            if (env.IsDevelopment())
            {
                app.UseDeveloperExceptionPage();
            }

            app.Run(async context =>
            {
                IActionResult result = null;

                try
                {
                    var url = context.Request.Path.Value.Trim('/');

                    if (url.StartsWith("http:") || url.StartsWith("https:"))
                    {
                        using (var cts = new CancellationTokenSource())
                        {
                            cts.CancelAfter(10 * 1000);

                            using (var request = new HttpRequestMessage(HttpMethod.Get, url))
                            using (var response = await Startup.httpClient.SendAsync(request, cts.Token).ConfigureAwait(false))
                            {
                                response.EnsureSuccessStatusCode();

                                var ret = await response.Content.ReadAsByteArrayAsync().ConfigureAwait(false);

                                result = new FileStreamResult(new MemoryStream(ret), response.Content.Headers.ContentType.ToString());
                            }
                        }
                    }
                }
                catch
                {
                }

                await (result ?? new NotFoundResult()).ExecuteResultAsync(new ActionContext
                {
                    HttpContext = context,
                    RouteData = result is RedirectResult ? new RouteData() : null
                }).ConfigureAwait(false);
            });
        }
    }
}
