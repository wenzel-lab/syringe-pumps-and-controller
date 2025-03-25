(function (factory) {
  typeof define === 'function' && define.amd ? define(factory) :
  factory();
})((function () { 'use strict';

  var tt$1=Object.defineProperty;var nt=(e,t,n)=>t in e?tt$1(e,t,{enumerable:!0,configurable:!0,writable:!0,value:n}):e[t]=n;var $$1=(e,t,n)=>(nt(e,typeof t!="symbol"?t+"":t,n),n);function st$1(e,t){const n=Object.create(null),s=e.split(",");for(let r=0;r<s.length;r++)n[s[r]]=!0;return t?r=>!!n[r.toLowerCase()]:r=>!!n[r]}function de(e){if(y$1(e)){const t={};for(let n=0;n<e.length;n++){const s=e[n],r=N$1(s)?it$1(s):de(s);if(r)for(const i in r)t[i]=r[i];}return t}else {if(N$1(e))return e;if(S$1(e))return e}}const rt$1=/;(?![^(]*\))/g,ot=/:(.+)/;function it$1(e){const t={};return e.split(rt$1).forEach(n=>{if(n){const s=n.split(ot);s.length>1&&(t[s[0].trim()]=s[1].trim());}}),t}function me(e){let t="";if(N$1(e))t=e;else if(y$1(e))for(let n=0;n<e.length;n++){const s=me(e[n]);s&&(t+=s+" ");}else if(S$1(e))for(const n in e)e[n]&&(t+=n+" ");return t.trim()}function ct(e,t){if(e.length!==t.length)return !1;let n=!0;for(let s=0;n&&s<e.length;s++)n=I$1(e[s],t[s]);return n}function I$1(e,t){if(e===t)return !0;let n=ge(e),s=ge(t);if(n||s)return n&&s?e.getTime()===t.getTime():!1;if(n=y$1(e),s=y$1(t),n||s)return n&&s?ct(e,t):!1;if(n=S$1(e),s=S$1(t),n||s){if(!n||!s)return !1;const r=Object.keys(e).length,i=Object.keys(t).length;if(r!==i)return !1;for(const c in e){const o=e.hasOwnProperty(c),l=t.hasOwnProperty(c);if(o&&!l||!o&&l||!I$1(e[c],t[c]))return !1}}return String(e)===String(t)}function G$1(e,t){return e.findIndex(n=>I$1(n,t))}const lt$1=Object.assign,ft=(e,t)=>{const n=e.indexOf(t);n>-1&&e.splice(n,1);},at=Object.prototype.hasOwnProperty,U$1=(e,t)=>at.call(e,t),y$1=Array.isArray,Y=e=>ye(e)==="[object Map]",ge=e=>e instanceof Date,N$1=e=>typeof e=="string",Q=e=>typeof e=="symbol",S$1=e=>e!==null&&typeof e=="object",ut=Object.prototype.toString,ye=e=>ut.call(e),pt=e=>ye(e).slice(8,-1),X$1=e=>N$1(e)&&e!=="NaN"&&e[0]!=="-"&&""+parseInt(e,10)===e,be=e=>{const t=Object.create(null);return n=>t[n]||(t[n]=e(n))},ht=/-(\w)/g,dt=be(e=>e.replace(ht,(t,n)=>n?n.toUpperCase():"")),mt=/\B([A-Z])/g,xe=be(e=>e.replace(mt,"-$1").toLowerCase()),gt=(e,t)=>!Object.is(e,t),ve=e=>{const t=parseFloat(e);return isNaN(t)?e:t};let yt;function we(e,t){t=t||yt,t&&t.active&&t.effects.push(e);}const _e=e=>{const t=new Set(e);return t.w=0,t.n=0,t},Ee=e=>(e.w&O$1)>0,$e=e=>(e.n&O$1)>0,bt=({deps:e})=>{if(e.length)for(let t=0;t<e.length;t++)e[t].w|=O$1;},xt=e=>{const{deps:t}=e;if(t.length){let n=0;for(let s=0;s<t.length;s++){const r=t[s];Ee(r)&&!$e(r)?r.delete(e):t[n++]=r,r.w&=~O$1,r.n&=~O$1;}t.length=n;}},ee=new WeakMap;let B=0,O$1=1;const te=30,z$1=[];let C$1;const W$1=Symbol(""),Se=Symbol("");class vt{constructor(t,n=null,s){this.fn=t,this.scheduler=n,this.active=!0,this.deps=[],we(this,s);}run(){if(!this.active)return this.fn();if(!z$1.includes(this))try{return z$1.push(C$1=this),$t(),O$1=1<<++B,B<=te?bt(this):Oe(this),this.fn()}finally{B<=te&&xt(this),O$1=1<<--B,ke(),z$1.pop();const t=z$1.length;C$1=t>0?z$1[t-1]:void 0;}}stop(){this.active&&(Oe(this),this.onStop&&this.onStop(),this.active=!1);}}function Oe(e){const{deps:t}=e;if(t.length){for(let n=0;n<t.length;n++)t[n].delete(e);t.length=0;}}function wt(e,t){e.effect&&(e=e.effect.fn);const n=new vt(e);t&&(lt$1(n,t),t.scope&&we(n,t.scope)),(!t||!t.lazy)&&n.run();const s=n.run.bind(n);return s.effect=n,s}function _t(e){e.effect.stop();}let K=!0;const ne=[];function Et(){ne.push(K),K=!1;}function $t(){ne.push(K),K=!0;}function ke(){const e=ne.pop();K=e===void 0?!0:e;}function F$1(e,t,n){if(!St())return;let s=ee.get(e);s||ee.set(e,s=new Map);let r=s.get(n);r||s.set(n,r=_e()),Ot(r);}function St(){return K&&C$1!==void 0}function Ot(e,t){let n=!1;B<=te?$e(e)||(e.n|=O$1,n=!Ee(e)):n=!e.has(C$1),n&&(e.add(C$1),C$1.deps.push(e));}function se(e,t,n,s,r,i){const c=ee.get(e);if(!c)return;let o=[];if(t==="clear")o=[...c.values()];else if(n==="length"&&y$1(e))c.forEach((l,f)=>{(f==="length"||f>=s)&&o.push(l);});else switch(n!==void 0&&o.push(c.get(n)),t){case"add":y$1(e)?X$1(n)&&o.push(c.get("length")):(o.push(c.get(W$1)),Y(e)&&o.push(c.get(Se)));break;case"delete":y$1(e)||(o.push(c.get(W$1)),Y(e)&&o.push(c.get(Se)));break;case"set":Y(e)&&o.push(c.get(W$1));break}if(o.length===1)o[0]&&Te(o[0]);else {const l=[];for(const f of o)f&&l.push(...f);Te(_e(l));}}function Te(e,t){for(const n of y$1(e)?e:[...e])(n!==C$1||n.allowRecurse)&&(n.scheduler?n.scheduler():n.run());}const kt=st$1("__proto__,__v_isRef,__isVue"),Ae=new Set(Object.getOwnPropertyNames(Symbol).map(e=>Symbol[e]).filter(Q)),Tt=Me(),At=Me(!0),Re=Rt();function Rt(){const e={};return ["includes","indexOf","lastIndexOf"].forEach(t=>{e[t]=function(...n){const s=j$1(this);for(let i=0,c=this.length;i<c;i++)F$1(s,"get",i+"");const r=s[t](...n);return r===-1||r===!1?s[t](...n.map(j$1)):r};}),["push","pop","shift","unshift","splice"].forEach(t=>{e[t]=function(...n){Et();const s=j$1(this)[t].apply(this,n);return ke(),s};}),e}function Me(e=!1,t=!1){return function(s,r,i){if(r==="__v_isReactive")return !e;if(r==="__v_isReadonly")return e;if(r==="__v_raw"&&i===(e?t?zt:je:t?Bt:Ce).get(s))return s;const c=y$1(s);if(!e&&c&&U$1(Re,r))return Reflect.get(Re,r,i);const o=Reflect.get(s,r,i);return (Q(r)?Ae.has(r):kt(r))||(e||F$1(s,"get",r),t)?o:re(o)?!c||!X$1(r)?o.value:o:S$1(o)?e?Ht(o):D$1(o):o}}const Mt=Ct();function Ct(e=!1){return function(n,s,r,i){let c=n[s];if(!e&&!Lt(r)&&(r=j$1(r),c=j$1(c),!y$1(n)&&re(c)&&!re(r)))return c.value=r,!0;const o=y$1(n)&&X$1(s)?Number(s)<n.length:U$1(n,s),l=Reflect.set(n,s,r,i);return n===j$1(i)&&(o?gt(r,c)&&se(n,"set",s,r):se(n,"add",s,r)),l}}function jt(e,t){const n=U$1(e,t);e[t];const s=Reflect.deleteProperty(e,t);return s&&n&&se(e,"delete",t,void 0),s}function Pt(e,t){const n=Reflect.has(e,t);return (!Q(t)||!Ae.has(t))&&F$1(e,"has",t),n}function It(e){return F$1(e,"iterate",y$1(e)?"length":W$1),Reflect.ownKeys(e)}const Nt={get:Tt,set:Mt,deleteProperty:jt,has:Pt,ownKeys:It},Kt={get:At,set(e,t){return !0},deleteProperty(e,t){return !0}},Ce=new WeakMap,Bt=new WeakMap,je=new WeakMap,zt=new WeakMap;function Dt(e){switch(e){case"Object":case"Array":return 1;case"Map":case"Set":case"WeakMap":case"WeakSet":return 2;default:return 0}}function Vt(e){return e.__v_skip||!Object.isExtensible(e)?0:Dt(pt(e))}function D$1(e){return e&&e.__v_isReadonly?e:Pe(e,!1,Nt,null,Ce)}function Ht(e){return Pe(e,!0,Kt,null,je)}function Pe(e,t,n,s,r){if(!S$1(e)||e.__v_raw&&!(t&&e.__v_isReactive))return e;const i=r.get(e);if(i)return i;const c=Vt(e);if(c===0)return e;const o=new Proxy(e,c===2?s:n);return r.set(e,o),o}function Lt(e){return !!(e&&e.__v_isReadonly)}function j$1(e){const t=e&&e.__v_raw;return t?j$1(t):e}function re(e){return Boolean(e&&e.__v_isRef===!0)}Promise.resolve();let oe=!1;const q$1=[],Wt=Promise.resolve(),V=e=>Wt.then(e),Ie=e=>{q$1.includes(e)||q$1.push(e),oe||(oe=!0,V(Ft));},Ft=()=>{for(const e of q$1)e();q$1.length=0,oe=!1;},qt=/^(spellcheck|draggable|form|list|type)$/,ie=({el:e,get:t,effect:n,arg:s,modifiers:r})=>{let i;s==="class"&&(e._class=e.className),n(()=>{let c=t();if(s)(r==null?void 0:r.camel)&&(s=dt(s)),ce(e,s,c,i);else {for(const o in c)ce(e,o,c[o],i&&i[o]);for(const o in i)(!c||!(o in c))&&ce(e,o,null);}i=c;});},ce=(e,t,n,s)=>{if(t==="class")e.setAttribute("class",me(e._class?[e._class,n]:n)||"");else if(t==="style"){n=de(n);const{style:r}=e;if(!n)e.removeAttribute("style");else if(N$1(n))n!==s&&(r.cssText=n);else {for(const i in n)le(r,i,n[i]);if(s&&!N$1(s))for(const i in s)n[i]==null&&le(r,i,"");}}else !(e instanceof SVGElement)&&t in e&&!qt.test(t)?(e[t]=n,t==="value"&&(e._value=n)):t==="true-value"?e._trueValue=n:t==="false-value"?e._falseValue=n:n!=null?e.setAttribute(t,n):e.removeAttribute(t);},Ne=/\s*!important$/,le=(e,t,n)=>{y$1(n)?n.forEach(s=>le(e,t,s)):t.startsWith("--")?e.setProperty(t,n):Ne.test(n)?e.setProperty(xe(t),n.replace(Ne,""),"important"):e[t]=n;},k$1=(e,t)=>{const n=e.getAttribute(t);return n!=null&&e.removeAttribute(t),n},T=(e,t,n,s)=>{e.addEventListener(t,n,s);},Jt=/^[A-Za-z_$][\w$]*(?:\.[A-Za-z_$][\w$]*|\['[^']*?']|\["[^"]*?"]|\[\d+]|\[[A-Za-z_$][\w$]*])*$/,Zt=["ctrl","shift","alt","meta"],Gt={stop:e=>e.stopPropagation(),prevent:e=>e.preventDefault(),self:e=>e.target!==e.currentTarget,ctrl:e=>!e.ctrlKey,shift:e=>!e.shiftKey,alt:e=>!e.altKey,meta:e=>!e.metaKey,left:e=>"button"in e&&e.button!==0,middle:e=>"button"in e&&e.button!==1,right:e=>"button"in e&&e.button!==2,exact:(e,t)=>Zt.some(n=>e[`${n}Key`]&&!t[n])},Ke=({el:e,get:t,exp:n,arg:s,modifiers:r})=>{if(!s)return;let i=Jt.test(n)?t(`(e => ${n}(e))`):t(`($event => { ${n} })`);if(s==="vue:mounted"){V(i);return}else if(s==="vue:unmounted")return ()=>i();if(r){s==="click"&&(r.right&&(s="contextmenu"),r.middle&&(s="mouseup"));const c=i;i=o=>{if(!("key"in o&&!(xe(o.key)in r))){for(const l in r){const f=Gt[l];if(f&&f(o,r))return}return c(o)}};}T(e,s,i,r);},Ut=({el:e,get:t,effect:n})=>{const s=e.style.display;n(()=>{e.style.display=t()?s:"none";});},Be=({el:e,get:t,effect:n})=>{n(()=>{e.textContent=ze(t());});},ze=e=>e==null?"":S$1(e)?JSON.stringify(e,null,2):String(e),Yt=({el:e,get:t,effect:n})=>{n(()=>{e.innerHTML=t();});},Qt=({el:e,exp:t,get:n,effect:s,modifiers:r})=>{const i=e.type,c=n(`(val) => { ${t} = val }`),{trim:o,number:l=i==="number"}=r||{};if(e.tagName==="SELECT"){const f=e;T(e,"change",()=>{const a=Array.prototype.filter.call(f.options,u=>u.selected).map(u=>l?ve(A$1(u)):A$1(u));c(f.multiple?a:a[0]);}),s(()=>{const a=n(),u=f.multiple;for(let p=0,x=f.options.length;p<x;p++){const b=f.options[p],v=A$1(b);if(u)y$1(a)?b.selected=G$1(a,v)>-1:b.selected=a.has(v);else if(I$1(A$1(b),a)){f.selectedIndex!==p&&(f.selectedIndex=p);return}}!u&&f.selectedIndex!==-1&&(f.selectedIndex=-1);});}else if(i==="checkbox"){T(e,"change",()=>{const a=n(),u=e.checked;if(y$1(a)){const p=A$1(e),x=G$1(a,p),b=x!==-1;if(u&&!b)c(a.concat(p));else if(!u&&b){const v=[...a];v.splice(x,1),c(v);}}else c(De(e,u));});let f;s(()=>{const a=n();y$1(a)?e.checked=G$1(a,A$1(e))>-1:a!==f&&(e.checked=I$1(a,De(e,!0))),f=a;});}else if(i==="radio"){T(e,"change",()=>{c(A$1(e));});let f;s(()=>{const a=n();a!==f&&(e.checked=I$1(a,A$1(e)));});}else {const f=a=>o?a.trim():l?ve(a):a;T(e,"compositionstart",Xt),T(e,"compositionend",en),T(e,(r==null?void 0:r.lazy)?"change":"input",()=>{e.composing||c(f(e.value));}),o&&T(e,"change",()=>{e.value=e.value.trim();}),s(()=>{if(e.composing)return;const a=e.value,u=n();document.activeElement===e&&f(a)===u||a!==u&&(e.value=u);});}},A$1=e=>"_value"in e?e._value:e.value,De=(e,t)=>{const n=t?"_trueValue":"_falseValue";return n in e?e[n]:t},Xt=e=>{e.target.composing=!0;},en=e=>{const t=e.target;t.composing&&(t.composing=!1,tn(t,"input"));},tn=(e,t)=>{const n=document.createEvent("HTMLEvents");n.initEvent(t,!0,!0),e.dispatchEvent(n);},Ve=Object.create(null),H=(e,t,n)=>He(e,`return(${t})`,n),He=(e,t,n)=>{const s=Ve[t]||(Ve[t]=nn(t));try{return s(e,n)}catch(r){console.error(r);}},nn=e=>{try{return new Function("$data","$el",`with($data){${e}}`)}catch(t){return console.error(`${t.message} in expression: ${e}`),()=>{}}},sn=({el:e,ctx:t,exp:n,effect:s})=>{V(()=>s(()=>He(t.scope,n,e)));},rn={bind:ie,on:Ke,show:Ut,text:Be,html:Yt,model:Qt,effect:sn},on=(e,t,n)=>{const s=e.parentElement,r=new Comment("v-if");s.insertBefore(r,e);const i=[{exp:t,el:e}];let c,o;for(;(c=e.nextElementSibling)&&(o=null,k$1(c,"v-else")===""||(o=k$1(c,"v-else-if")));)s.removeChild(c),i.push({exp:o,el:c});const l=e.nextSibling;s.removeChild(e);let f,a=-1;const u=()=>{f&&(s.insertBefore(r,f.el),f.remove(),f=void 0);};return n.effect(()=>{for(let p=0;p<i.length;p++){const{exp:x,el:b}=i[p];if(!x||H(n.scope,x)){p!==a&&(u(),f=new ue(b,n),f.insert(s,r),s.removeChild(r),a=p);return}}a=-1,u();}),l},cn=/([\s\S]*?)\s+(?:in|of)\s+([\s\S]*)/,Le=/,([^,\}\]]*)(?:,([^,\}\]]*))?$/,ln=/^\(|\)$/g,fn=/^[{[]\s*((?:[\w_$]+\s*,?\s*)+)[\]}]$/,an=(e,t,n)=>{const s=t.match(cn);if(!s)return;const r=e.nextSibling,i=e.parentElement,c=new Text("");i.insertBefore(c,e),i.removeChild(e);const o=s[2].trim();let l=s[1].trim().replace(ln,"").trim(),f,a=!1,u,p,x="key",b=e.getAttribute(x)||e.getAttribute(x=":key")||e.getAttribute(x="v-bind:key");b&&(e.removeAttribute(x),x==="key"&&(b=JSON.stringify(b)));let v;(v=l.match(Le))&&(l=l.replace(Le,"").trim(),u=v[1].trim(),v[2]&&(p=v[2].trim())),(v=l.match(fn))&&(f=v[1].split(",").map(m=>m.trim()),a=l[0]==="[");let pe=!1,R,L,J;const et=m=>{const w=new Map,h=[];if(y$1(m))for(let d=0;d<m.length;d++)h.push(Z(w,m[d],d));else if(typeof m=="number")for(let d=0;d<m;d++)h.push(Z(w,d+1,d));else if(S$1(m)){let d=0;for(const g in m)h.push(Z(w,m[g],d++,g));}return [h,w]},Z=(m,w,h,d)=>{const g={};f?f.forEach((M,E)=>g[M]=w[a?E:M]):g[l]=w,d?(u&&(g[u]=d),p&&(g[p]=h)):u&&(g[u]=h);const P=Ge(n,g),_=b?H(P.scope,b):h;return m.set(_,h),P.key=_,P},he=(m,w)=>{const h=new ue(e,m);return h.key=m.key,h.insert(i,w),h};return n.effect(()=>{const m=H(n.scope,o),w=J;if([L,J]=et(m),!pe)R=L.map(h=>he(h,c)),pe=!0;else {for(let _=0;_<R.length;_++)J.has(R[_].key)||R[_].remove();const h=[];let d=L.length,g,P;for(;d--;){const _=L[d],M=w.get(_.key);let E;M==null?E=he(_,g?g.el:c):(E=R[M],Object.assign(E.ctx.scope,_.scope),M!==d&&(R[M+1]!==g||P===g)&&(P=E,E.insert(i,g?g.el:c))),h.unshift(g=E);}R=h;}}),r},We=({el:e,ctx:{scope:{$refs:t}},get:n,effect:s})=>{let r;return s(()=>{const i=n();t[i]=e,r&&i!==r&&delete t[r],r=i;}),()=>{r&&delete t[r];}},un=/^(?:v-|:|@)/,pn=/\.([\w-]+)/g;let fe=!1;const Fe=(e,t)=>{const n=e.nodeType;if(n===1){const s=e;if(s.hasAttribute("v-pre"))return;k$1(s,"v-cloak");let r;if(r=k$1(s,"v-if"))return on(s,r,t);if(r=k$1(s,"v-for"))return an(s,r,t);if((r=k$1(s,"v-scope"))||r===""){const o=r?H(t.scope,r):{};t=Ge(t,o),o.$template&&hn(s,o.$template);}const i=k$1(s,"v-once")!=null;i&&(fe=!0),(r=k$1(s,"ref"))&&ae(s,We,`"${r}"`,t),qe(s,t);const c=[];for(const{name:o,value:l}of [...s.attributes])un.test(o)&&o!=="v-cloak"&&(o==="v-model"?c.unshift([o,l]):o[0]==="@"||/^v-on\b/.test(o)?c.push([o,l]):Je(s,o,l,t));for(const[o,l]of c)Je(s,o,l,t);i&&(fe=!1);}else if(n===3){const s=e.data;if(s.includes(t.delimiters[0])){let r=[],i=0,c;for(;c=t.delimitersRE.exec(s);){const o=s.slice(i,c.index);o&&r.push(JSON.stringify(o)),r.push(`$s(${c[1]})`),i=c.index+c[0].length;}i<s.length&&r.push(JSON.stringify(s.slice(i))),ae(e,Be,r.join("+"),t);}}else n===11&&qe(e,t);},qe=(e,t)=>{let n=e.firstChild;for(;n;)n=Fe(n,t)||n.nextSibling;},Je=(e,t,n,s)=>{let r,i,c;if(t=t.replace(pn,(o,l)=>((c||(c={}))[l]=!0,"")),t[0]===":")r=ie,i=t.slice(1);else if(t[0]==="@")r=Ke,i=t.slice(1);else {const o=t.indexOf(":"),l=o>0?t.slice(2,o):t.slice(2);r=rn[l]||s.dirs[l],i=o>0?t.slice(o+1):void 0;}r&&(r===ie&&i==="ref"&&(r=We),ae(e,r,n,s,i,c),e.removeAttribute(t));},ae=(e,t,n,s,r,i)=>{const c=t({el:e,get:(o=n)=>H(s.scope,o,e),effect:s.effect,ctx:s,exp:n,arg:r,modifiers:i});c&&s.cleanups.push(c);},hn=(e,t)=>{if(t[0]==="#"){const n=document.querySelector(t);e.appendChild(n.content.cloneNode(!0));return}e.innerHTML=t;},Ze=e=>{const t={delimiters:["{{","}}"],delimitersRE:/\{\{([^]+?)\}\}/g,...e,scope:e?e.scope:D$1({}),dirs:e?e.dirs:{},effects:[],blocks:[],cleanups:[],effect:n=>{if(fe)return Ie(n),n;const s=wt(n,{scheduler:()=>Ie(s)});return t.effects.push(s),s}};return t},Ge=(e,t={})=>{const n=e.scope,s=Object.create(n);Object.defineProperties(s,Object.getOwnPropertyDescriptors(t)),s.$refs=Object.create(n.$refs);const r=D$1(new Proxy(s,{set(i,c,o,l){return l===r&&!i.hasOwnProperty(c)?Reflect.set(n,c,o):Reflect.set(i,c,o,l)}}));return Ue(r),{...e,scope:r}},Ue=e=>{for(const t of Object.keys(e))typeof e[t]=="function"&&(e[t]=e[t].bind(e));};class ue{constructor(t,n,s=!1){$$1(this,"template");$$1(this,"ctx");$$1(this,"key");$$1(this,"parentCtx");$$1(this,"isFragment");$$1(this,"start");$$1(this,"end");this.isFragment=t instanceof HTMLTemplateElement,s?this.template=t:this.isFragment?this.template=t.content.cloneNode(!0):this.template=t.cloneNode(!0),s?this.ctx=n:(this.parentCtx=n,n.blocks.push(this),this.ctx=Ze(n)),Fe(this.template,this.ctx);}get el(){return this.start||this.template}insert(t,n=null){if(this.isFragment)if(this.start){let s=this.start,r;for(;s&&(r=s.nextSibling,t.insertBefore(s,n),s!==this.end);)s=r;}else this.start=new Text(""),this.end=new Text(""),t.insertBefore(this.end,n),t.insertBefore(this.start,this.end),t.insertBefore(this.template,this.end);else t.insertBefore(this.template,n);}remove(){if(this.parentCtx&&ft(this.parentCtx.blocks,this),this.start){const t=this.start.parentNode;let n=this.start,s;for(;n&&(s=n.nextSibling,t.removeChild(n),n!==this.end);)n=s;}else this.template.parentNode.removeChild(this.template);this.teardown();}teardown(){this.ctx.blocks.forEach(t=>{t.teardown();}),this.ctx.effects.forEach(_t),this.ctx.cleanups.forEach(t=>t());}}const Ye=e=>e.replace(/[-.*+?^${}()|[\]\/\\]/g,"\\$&"),Qe=e=>{const t=Ze();if(e&&(t.scope=D$1(e),Ue(t.scope),e.$delimiters)){const[s,r]=t.delimiters=e.$delimiters;t.delimitersRE=new RegExp(Ye(s)+"([^]+?)"+Ye(r),"g");}t.scope.$s=ze,t.scope.$nextTick=V,t.scope.$refs=Object.create(null);let n;return {directive(s,r){return r?(t.dirs[s]=r,this):t.dirs[s]},mount(s){if(typeof s=="string"&&(s=document.querySelector(s),!s))return;s=s||document.documentElement;let r;return s.hasAttribute("v-scope")?r=[s]:r=[...s.querySelectorAll("[v-scope]")].filter(i=>!i.matches("[v-scope] [v-scope]")),r.length||(r=[s]),n=r.map(i=>new ue(i,t,!0)),this},unmount(){n.forEach(s=>s.teardown());}}},Xe=document.currentScript;Xe&&Xe.hasAttribute("init")&&Qe().mount();

  /** @license
   * fzf v0.5.1
   * Copyright (c) 2021-2022 Ajit
   * Licensed under BSD 3-Clause
   */
  const t={216:"O",223:"s",248:"o",273:"d",295:"h",305:"i",320:"l",322:"l",359:"t",383:"s",384:"b",385:"B",387:"b",390:"O",392:"c",393:"D",394:"D",396:"d",398:"E",400:"E",402:"f",403:"G",407:"I",409:"k",410:"l",412:"M",413:"N",414:"n",415:"O",421:"p",427:"t",429:"t",430:"T",434:"V",436:"y",438:"z",477:"e",485:"g",544:"N",545:"d",549:"z",564:"l",565:"n",566:"t",567:"j",570:"A",571:"C",572:"c",573:"L",574:"T",575:"s",576:"z",579:"B",580:"U",581:"V",582:"E",583:"e",584:"J",585:"j",586:"Q",587:"q",588:"R",589:"r",590:"Y",591:"y",592:"a",593:"a",595:"b",596:"o",597:"c",598:"d",599:"d",600:"e",603:"e",604:"e",605:"e",606:"e",607:"j",608:"g",609:"g",610:"G",613:"h",614:"h",616:"i",618:"I",619:"l",620:"l",621:"l",623:"m",624:"m",625:"m",626:"n",627:"n",628:"N",629:"o",633:"r",634:"r",635:"r",636:"r",637:"r",638:"r",639:"r",640:"R",641:"R",642:"s",647:"t",648:"t",649:"u",651:"v",652:"v",653:"w",654:"y",655:"Y",656:"z",657:"z",663:"c",665:"B",666:"e",667:"G",668:"H",669:"j",670:"k",671:"L",672:"q",686:"h",867:"a",868:"e",869:"i",870:"o",871:"u",872:"c",873:"d",874:"h",875:"m",876:"r",877:"t",878:"v",879:"x",7424:"A",7427:"B",7428:"C",7429:"D",7431:"E",7432:"e",7433:"i",7434:"J",7435:"K",7436:"L",7437:"M",7438:"N",7439:"O",7440:"O",7441:"o",7442:"o",7443:"o",7446:"o",7447:"o",7448:"P",7449:"R",7450:"R",7451:"T",7452:"U",7453:"u",7454:"u",7455:"m",7456:"V",7457:"W",7458:"Z",7522:"i",7523:"r",7524:"u",7525:"v",7834:"a",7835:"s",8305:"i",8341:"h",8342:"k",8343:"l",8344:"m",8345:"n",8346:"p",8347:"s",8348:"t",8580:"c"};for(let at="̀".codePointAt(0);at<="ͯ".codePointAt(0);++at){const e=String.fromCodePoint(at);for(const n of "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"){const r=(n+e).normalize().codePointAt(0);r>126&&(t[r]=n);}}const e={a:[7844,7863],e:[7870,7879],o:[7888,7907],u:[7912,7921]};for(const at of Object.keys(e)){const n=at.toUpperCase();for(let r=e[at][0];r<=e[at][1];++r)t[r]=r%2==0?n:at;}function n(e){if(e<192||e>8580)return e;const n=t[e];return void 0!==n?n.codePointAt(0):e}function r(t,e){return t>e?t:e}const s=t=>t.split("").map((t=>t.codePointAt(0))),i=new Set(" \f\n\r\t\v  \u2028\u2029  　\ufeff".split("").map((t=>t.codePointAt(0))));for(let at=" ".codePointAt(0);at<=" ".codePointAt(0);at++)i.add(at);const u="".codePointAt(0),f="A".codePointAt(0),h="Z".codePointAt(0),d="a".codePointAt(0),g="z".codePointAt(0),m="0".codePointAt(0),p="9".codePointAt(0);function b(t,e,n){return n?t:e-t-1}var y,P;function w(t){return t?new Set:null}function z(t,e,n){if(null!==e&&e.i16.length>t+n){return [t+n,e.i16.subarray(t,t+n)]}return [t,new Int16Array(n)]}function L(t,e,n){if(null!==e&&e.i32.length>t+n){return [t+n,e.i32.subarray(t,t+n)]}return [t,new Int32Array(n)]}function S(t){return t>=d&&t<=g?1:t>=f&&t<=h?2:t>=m&&t<=p?4:0}function A(t){const e=String.fromCodePoint(t);return e!==e.toUpperCase()?1:e!==e.toLowerCase()?2:null!==e.match(/\p{Number}/gu)?4:null!==e.match(/\p{Letter}/gu)?3:0}function C(t){return t<=u?S(t):A(t)}function v(t,e){return 0===t&&0!==e?8:1===t&&2===e||4!==t&&4===e?7:0===e?8:0}function k(t,e,n,r){let s=t.slice(r),o=s.indexOf(n);if(0===o)return r;if(!e&&n>=d&&n<=g){o>0&&(s=s.slice(0,o));const t=s.indexOf(n-32);t>=0&&(o=t);}return o<0?-1:r+o}function x(t){for(const e of t)if(e>=128)return !1;return !0}function E(t,e,n){if(!x(t))return 0;if(!x(e))return -1;let r=0,s=0;for(let o=0;o<e.length;o++){if(s=k(t,n,e[o],s),s<0)return -1;0===o&&s>0&&(r=s-1),s++;}return r}(P=y||(y={}))[P.NonWord=0]="NonWord",P[P.Lower=1]="Lower",P[P.Upper=2]="Upper",P[P.Letter=3]="Letter",P[P.Number=4]="Number";const N=(t,e,s,o,i,l,c)=>{const a=i.length;if(0===a)return [{start:0,end:0,score:0},w(l)];const f=o.length;if(null!==c&&f*a>c.i16.length)return O(t,e,s,o,i,l);const h=E(o,i,t);if(h<0)return [{start:-1,end:-1,score:0},null];let d=0,g=0,m=null,p=null,b=null,y=null;[d,m]=z(d,c,f),[d,p]=z(d,c,f),[d,b]=z(d,c,f),[g,y]=L(g,c,a);const[,P]=L(g,c,f);for(let n=0;n<P.length;n++)P[n]=o[n];let C=0,k=0,x=0,N=0;const F=i[0];let R=i[0],q=0,I=0,W=!1,j=P.subarray(h),M=m.subarray(h).subarray(0,j.length),B=p.subarray(h).subarray(0,j.length),U=b.subarray(h).subarray(0,j.length);for(let[w,z]of j.entries()){let o=null;z<=u?(o=S(z),t||2!==o||(z+=32)):(o=A(z),t||2!==o||(z=String.fromCodePoint(z).toLowerCase().codePointAt(0)),e&&(z=n(z))),j[w]=z;const l=v(I,o);if(U[w]=l,I=o,z===R&&(x<a&&(y[x]=h+w,x++,R=i[Math.min(x,a-1)]),N=h+w),z===F){const t=16+2*l;if(M[w]=t,B[w]=1,1===a&&(s&&t>C||!s&&t>=C)&&(C=t,k=h+w,s&&8===l))break;W=!1;}else M[w]=r(W?q+-1:q+-3,0),B[w]=0,W=!0;q=M[w];}if(x!==a)return [{start:-1,end:-1,score:0},null];if(1===a){const t={start:k,end:k+1,score:C};if(!l)return [t,null];const e=new Set;return e.add(k),[t,e]}const T=y[0],D=N-T+1;let G=null;[d,G]=z(d,c,D*a);{const t=m.subarray(T,N+1);for(const[e,n]of t.entries())G[e]=n;}let[,V]=z(d,c,D*a);{const t=p.subarray(T,N+1);for(const[e,n]of t.entries())V[e]=n;}const J=y.subarray(1),Y=i.slice(1).slice(0,J.length);for(const[n,u]of J.entries()){let t=!1;const e=Y[n],o=n+1,i=o*D,l=P.subarray(u,N+1),c=b.subarray(u).subarray(0,l.length),f=V.subarray(i+u-T).subarray(0,l.length),h=V.subarray(i+u-T-1-D).subarray(0,l.length),d=G.subarray(i+u-T).subarray(0,l.length),g=G.subarray(i+u-T-1-D).subarray(0,l.length),m=G.subarray(i+u-T-1).subarray(0,l.length);m[0]=0;for(const[n,p]of l.entries()){const i=n+u;let l=0,y=0,P=0;if(y=t?m[n]+-1:m[n]+-3,e===p){l=g[n]+16;let t=c[n];P=h[n]+1,8===t?P=1:P>1&&(t=r(t,r(4,b[i-P+1]))),l+t<y?(l+=c[n],P=0):l+=t;}f[n]=P,t=l<y;const w=r(r(l,y),0);o===a-1&&(s&&w>C||!s&&w>=C)&&(C=w,k=i),d[n]=w;}}const Z=w(l);let H=T;if(l&&null!==Z){let t=a-1;H=k;let e=!0;for(;;){const n=t*D,r=H-T,s=G[n+r];let o=0,i=0;if(t>0&&H>=y[t]&&(o=G[n-D+r-1]),H>y[t]&&(i=G[n+r-1]),s>o&&(s>i||s===i&&e)){if(Z.add(H),0===t)break;t--;}e=V[n+r]>1||n+D+r+1<V.length&&V[n+D+r+1]>0,H--;}}return [{start:H,end:k+1,score:C},Z]};function F(t,e,s,o,i,l,c){let a=0,d=0,g=!1,m=0,p=0;const b=w(c);let y=0;i>0&&(y=C(s[i-1]));for(let P=i;P<l;P++){let i=s[P];const l=C(i);if(t||(i>=f&&i<=h?i+=32:i>u&&(i=String.fromCodePoint(i).toLowerCase().codePointAt(0))),e&&(i=n(i)),i===o[a]){c&&null!==b&&b.add(P),d+=16;let t=v(y,l);0===m?p=t:(8===t&&(p=t),t=r(r(t,p),4)),d+=0===a?2*t:t,g=!1,m++,a++;}else d+=g?-1:-3,g=!0,m=0,p=0;y=l;}return [d,b]}const O=(t,e,r,s,o,i,l)=>{if(0===o.length)return [{start:0,end:0,score:0},null];if(E(s,o,t)<0)return [{start:-1,end:-1,score:0},null];let c=0,a=-1,d=-1;const g=s.length,m=o.length;for(let p=0;p<g;p++){let i=s[b(p,g,r)];t||(i>=f&&i<=h?i+=32:i>u&&(i=String.fromCodePoint(i).toLowerCase().codePointAt(0))),e&&(i=n(i));if(i===o[b(c,m,r)]&&(a<0&&(a=p),c++,c===m)){d=p+1;break}}if(a>=0&&d>=0){c--;for(let e=d-1;e>=a;e--){let n=s[b(e,g,r)];t||(n>=f&&n<=h?n+=32:n>u&&(n=String.fromCodePoint(n).toLowerCase().codePointAt(0)));if(n===o[b(c,m,r)]&&(c--,c<0)){a=e;break}}if(!r){const t=a;a=g-d,d=g-t;}const[n,l]=F(t,e,s,o,a,d,i);return [{start:a,end:d,score:n},l]}return [{start:-1,end:-1,score:0},null]},R=(t,e,r,s,o,i,l)=>{if(0===o.length)return [{start:0,end:0,score:0},null];const c=s.length,a=o.length;if(c<a)return [{start:-1,end:-1,score:0},null];if(E(s,o,t)<0)return [{start:-1,end:-1,score:0},null];let d=0,g=-1,m=0,p=-1;for(let w=0;w<c;w++){const i=b(w,c,r);let l=s[i];t||(l>=f&&l<=h?l+=32:l>u&&(l=String.fromCodePoint(l).toLowerCase().codePointAt(0))),e&&(l=n(l));const z=b(d,a,r);if(o[z]===l){if(0===z&&(y=s,m=0===(P=i)?8:v(C(y[P-1]),C(y[P]))),d++,d===a){if(m>p&&(g=w,p=m),8===m)break;w-=d-1,d=0,m=0;}}else w-=d,d=0,m=0;}var y,P;if(g>=0){let n=0,i=0;r?(n=g-a+1,i=g+1):(n=c-(g+1),i=c-(g-a+1));const[l]=F(t,e,s,o,n,i,!1);return [{start:n,end:i,score:l},null]}return [{start:-1,end:-1,score:0},null]};const q=(I=2048,{i16:new Int16Array(102400),i32:new Int32Array(I)});var I,W,j;(j=W||(W={}))[j.Fuzzy=0]="Fuzzy",j[j.Exact=1]="Exact",j[j.Prefix=2]="Prefix",j[j.Suffix=3]="Suffix",j[j.Equal=4]="Equal";const U=(t,e,r)=>{let o=!1;switch(e){case"smart-case":t.toLowerCase()!==t&&(o=!0);break;case"case-sensitive":o=!0;break;case"case-insensitive":t=t.toLowerCase(),o=!1;}let i=s(t);return r&&(i=i.map(n)),{queryRunes:i,caseSensitive:o}};function D(t,e){const n=Object.keys(t).map((t=>parseInt(t,10))).sort(((t,e)=>e-t));let r=[];for(const s of n)if(r=r.concat(t[s]),r.length>=e)break;return r}function G(t,e,n){return r=>{const s=this.runesList[r];if(e.length>s.length)return;let[o,i]=this.algoFn(n,this.opts.normalize,this.opts.forward,s,e,!0,q);if(-1===o.start)return;if(!1===this.opts.fuzzy){i=new Set;for(let t=o.start;t<o.end;++t)i.add(t);}const l=this.opts.sort?o.score:0;void 0===t[l]&&(t[l]=[]),t[l].push({item:this.items[r],...o,positions:null!=i?i:new Set});}}function J(t){const{queryRunes:e,caseSensitive:n}=U(t,this.opts.casing,this.opts.normalize),r={},s=G.bind(this)(r,e,n);for(let o=0,i=this.runesList.length;o<i;++o)s(o);return D(r,this.opts.limit)}const $={limit:1/0,selector:t=>t,casing:"smart-case",normalize:!0,fuzzy:"v2",tiebreakers:[],sort:!0,forward:!0};class X{constructor(t,...e){switch(this.opts={...$,...e[0]},this.items=t,this.runesList=t.map((t=>s(this.opts.selector(t).normalize()))),this.algoFn=R,this.opts.fuzzy){case"v2":this.algoFn=N;break;case"v1":this.algoFn=O;}}}const _={...$,match:J};class tt extends X{constructor(t,...e){super(t,...e),this.opts={..._,...e[0]};}find(t){if(0===t.length||0===this.items.length)return this.items.slice(0,this.opts.limit).map(rt);return t=t.normalize(),st(this.opts.match.bind(this)(t),this.opts)}}const rt=t=>({item:t,start:-1,end:-1,score:0,positions:new Set});function st(t,e){if(e.sort){const{selector:n}=e;t.sort(((t,r)=>{if(t.score===r.score)for(const s of e.tiebreakers){const e=s(t,r,n);if(0!==e)return e}return 0}));}return Number.isFinite(e.limit)&&t.splice(e.limit),t}function it(t,e){return t.start-e.start}class lt{constructor(t,...e){this.finder=new tt(t,...e),this.find=this.finder.find.bind(this.finder);}}

  Qe({
    searchTerm: getSessionSearchTerm(),
    searchResults: [],
    index: null,
    exactIndex: null,
    focused: false,
    focusTimeout: null,
    rootPath: "./",
    mounted(root) {
      this.rootPath = root;
      fetch(root + "search_index.json")
        .then((r) => r.json())
        .then((searchIndexJson) => {
          searchIndexJson = searchIndexJson.map((doc) => ({
            ...doc,
            breadCrumbs: getBreadCrumbs(doc.path, searchIndexJson),
          }));
          this.index = new lt(searchIndexJson, {
            limit: 10,
            tiebreakers: [it],
            selector: (doc) => doc.content,
          });
          this.exactIndex = new lt(searchIndexJson, {
            limit: 10,
            tiebreakers: [it],
            selector: (doc) => doc.content,
            fuzzy: false,
          });
          const term = getSessionSearchTerm();
          this.search(term);
        });
      // can't seem to get this from v-on:keypress on the input element
      document.addEventListener("keydown", (event) => {
        if (event.key === "Escape") {
          document.querySelector("#nav-search input").blur();
          this.focused = false;
        }
      });
    },
    search(term) {
      setSessionSearchTerm(term);
      this.searchTerm = term;
      if (term && this.index) {
        let results = find(this.index, term);
        if (results.length === 0) {
          results = find(this.exactIndex, term);
        }
        this.searchResults = results;
      }
    },
    handleKeyPress(event) {
      if (event.key === "Enter") {
        event.target.blur();
        this.focused = false;
        if (this.searchResults.length > 0) {
          window.location = this.rootPath + this.searchResults[0].path + ".html";
        }
      }
    },
    handleFocus() {
      clearTimeout(this.focusTimeout);
      this.focused = true;
    },
    handleBlur() {
      // remove focus after a delay so the user has time to click on links in the
      // popover
      clearTimeout(this.focusTimeout);
      this.focusTimeout = setTimeout(() => {
        this.focused = false;
      }, 200);
    },
    $delimiters: ["${", "}"],
  }).mount("#nav-search");

  function find(index, term) {
    // runs the search on the given index and computes highlight areas in the content
    let results = index.find(term);
    results = results.map((r) => ({
      ...r.item,
      highlightChars: computeHighlightChars(r.item.content, r.positions),
    }));
    // get rid results that only have matches of less than 3 characters
    // (unless search term is also less than 3 characters)
    results = results.filter((r) => {
      for (const node of r.highlightChars) {
        if (node.mark && node.chars.length >= Math.min(3, term.length)) {
          return true;
        }
      }
    });
    // get rid of highlights that are less than 3 characters (unless the
    // search term is also less than 3), they don't seem useful to the user
    results = results.map((r) => {
      return {
        ...r,
        highlightChars: r.highlightChars.map((node) =>
          node.mark && node.chars.length < Math.min(3, term.length)
            ? { ...node, mark: false }
            : node
        ),
      };
    });
    return results;
  }

  function getSessionSearchTerm() {
    return sessionStorage.getItem(`searchTerm`) || "";
  }

  function setSessionSearchTerm(term) {
    setTimeout(() => {
      sessionStorage.setItem(`searchTerm`, term);
    }, 0);
  }

  const CONTEXT = 30;
  function computeHighlightChars(content, positions) {
    // seperates content into "nodes" of highlighted (`mark: true`) and
    // non-highlighted (`mark: false`) content. removes non-highlighted content
    // outside of the `CONTEXT` range and adds ellipses ("...")
    const chars = content.split("");

    const nodes = chars.reduce((prev, cur, i) => {
      const mark = positions.has(i);
      if (prev.length === 0) {
        return [{ mark, chars: cur }];
      }
      const last = prev[prev.length - 1];
      if (last.mark === mark) {
        last.chars += cur;
        return prev;
      }
      return prev.concat([{ mark, chars: cur }]);
    }, []);

    return nodes.map((node, i) => {
      if (node.mark) {
        return node;
      }
      if (node.chars.length <= CONTEXT * 2) {
        return node;
      }
      const last = nodes[i - 1];
      const next = nodes[i + 1];
      if (!last) {
        return { ...node, chars: "..." + node.chars.slice(-CONTEXT) };
      }
      if (!next) {
        return {
          ...node,
          chars: node.chars.slice(0, CONTEXT) + "...",
        };
      }
      return {
        ...node,
        chars: node.chars.slice(0, CONTEXT) + "..." + node.chars.slice(-CONTEXT),
      };
    }, []);
  }

  function getBreadCrumbs(filepath, index) {
    // returns arrays like `['Project Page', 'project_sub_page']`. determined from
    // the folder structure and titles looked up in the search index
    const split = splitPaths(filepath);
    for (const doc of index) {
      if (doc.path === split[0] && doc.title != null) {
        split[0] = doc.title;
        return split;
      }
    }
    return split;
  }

  function splitPaths(filepath) {
    if (filepath.endsWith("_BOM")) {
      return filepath.split(/_BOM$/)[0].split("/").concat(["BOM"]);
    }
    return filepath.split("/");
  }

}));
